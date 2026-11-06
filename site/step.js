// Rucas one instruction at a time, a twin of rucas.cpp, so the page can run slowly and show every step.

const OPS = [
  'ADD',
  'SUB',
  'JMPR',
  'JMPL',
  'JMP',
  'PRNTA',
  'PRNTD',
  'CINP',
  'STLP',
  'NDLP',
  'IF',
  'IF EQL',
  'ENDIF',
  'HALT',
  'PSHS',
  'CLRS',
  'RMVS',
  'EQL TOPS',
  'ADDS',
  'SUBS',
  'MULS',
  'DIVS',
  'MODS',
  'ELSE',
  'IF LESS',
  'IF MORE',
];
const OPENERS = ['STLP', 'IF', 'IF EQL', 'IF LESS', 'IF MORE'];
const TAPE = 30000;
const NONE = 0,
  NUMBER = 1,
  CELL = 2,
  STACK = 3;
const KINDS = { none: NONE, n: NUMBER, $CELL: CELL, $STACK: STACK };

class RucasError extends Error {}

function parseArg(text) {
  if (text === '') return { arg: 'none', n: 1 };
  if (text === '$CELL' || text === '$STACK') return { arg: text, n: 1 };
  if (text.length > 10 || !/^[0-9]+$/.test(text) || Number(text) > 0xffffffff) return null;
  return { arg: 'n', n: Number(text) };
}

class Machine {
  constructor(files, input) {
    this.names = files.map((f) => f.name);
    this.mask = 0xff;
    this.program = [];
    this.parse(files);
    // plain number arrays, so the loop in run() never touches an object or a string
    this.ops = Int32Array.from(this.program, (ins) => OPS.indexOf(ins.op));
    this.kinds = Int32Array.from(this.program, (ins) => KINDS[ins.arg]);
    this.ns = Uint32Array.from(this.program, (ins) => ins.n);
    this.matches = Int32Array.from(this.program, (ins) => ins.match);
    this.out = [];
    this.tape = new Uint32Array(TAPE);
    this.cell = 0;
    // the stack lives in stack[lo] to stack[hi - 1], so CLRS and RMVS only move a number
    this.stack = new Float64Array(1024);
    this.lo = 0;
    this.hi = 0;
    this.input = new TextEncoder().encode(input);
    this.read = 0;
    this.at = 0;
    this.done = this.program.length === 0;
  }

  where(file, line) {
    return (this.names.length > 1 ? `${this.names[file]} ` : '') + `line ${line}`;
  }

  fail(file, line, message) {
    throw new RucasError(`error: ${this.where(file, line)}: ${message}`);
  }

  // the same checks and messages as rucas.cpp, so both say the same about a broken program
  parse(files) {
    const open = [];
    files.forEach((f, file) => {
      const lines = f.text.split('\n');
      if (lines[lines.length - 1] === '') lines.pop();
      lines.forEach((text, k) => this.parseLine(text, file, k + 1, open));
    });
    if (open.length) {
      const ins = this.program[open[open.length - 1]];
      this.fail(ins.file, ins.line, `${ins.op} is never closed`);
    }
  }

  parseLine(text, file, line, open) {
    const words = text
      .split('#')[0]
      .split(/[ \t\r\v\f]+/)
      .filter(Boolean);
    if (!words.length) return;
    let op = words.shift();
    if (
      (op === 'IF' && ['EQL', 'LESS', 'MORE'].includes(words[0])) ||
      (op === 'EQL' && words[0] === 'TOPS')
    )
      op += ' ' + words.shift();
    const arg = words.shift() || '';
    if (words.length) this.fail(file, line, `unexpected '${words[0]}'`);
    if (op === 'BITS') return this.setBits(arg, file, line);
    if (!OPS.includes(op)) this.fail(file, line, `unknown instruction '${op}'`);
    const value = parseArg(arg);
    if (!value)
      this.fail(file, line, `'${arg}' is not a valid argument (expected N, $CELL or $STACK)`);
    const ins = { op, text: arg, ...value, file, line, match: -1 };
    if (['JMP', 'IF EQL', 'IF LESS', 'IF MORE'].includes(op) && ins.arg === 'none')
      this.fail(
        file,
        line,
        op + (op === 'JMP' ? ' needs a cell number' : ' needs a value to compare with'),
      );
    this.match(ins, open);
    this.program.push(ins);
  }

  setBits(arg, file, line) {
    if (this.program.length) this.fail(file, line, 'BITS must come before the first instruction');
    const value = parseArg(arg);
    if (!value || value.arg !== 'n' || ![8, 16, 32].includes(value.n))
      this.fail(file, line, 'BITS takes 8, 16 or 32');
    this.mask = value.n === 32 ? 0xffffffff : (1 << value.n) - 1;
  }

  // a false IF jumps to its ELSE, and the ELSE jumps to ENDIF
  match(ins, open) {
    const index = this.program.length;
    const top = open[open.length - 1];
    const inIf = open.length > 0 && this.program[top].op !== 'STLP';
    if (OPENERS.includes(ins.op)) return open.push(index);
    if (ins.op === 'ELSE') {
      if (!inIf || this.program[top].op === 'ELSE')
        this.fail(ins.file, ins.line, 'ELSE without matching IF');
      this.program[top].match = index;
      ins.match = top;
      open[open.length - 1] = index;
      return;
    }
    if (ins.op !== 'NDLP' && ins.op !== 'ENDIF') return;
    const loop = ins.op === 'NDLP';
    if (!open.length || inIf === loop)
      this.fail(ins.file, ins.line, `${ins.op} without matching ${loop ? 'STLP' : 'IF'}`);
    this.program[top].match = index;
    ins.match = top;
    open.pop();
  }

  // one long loop on purpose, with no call per instruction, since every run below Full goes through it
  run(count) {
    const ops = this.ops,
      kinds = this.kinds,
      ns = this.ns,
      matches = this.matches;
    const t = this.tape,
      m = this.mask,
      end = this.program.length,
      out = this.out;
    let at = this.at,
      cell = this.cell,
      k = 0;
    let st = this.stack,
      lo = this.lo,
      hi = this.hi;
    try {
      for (; k < count && at < end; k++) {
        const kind = kinds[at];
        const v = kind === CELL ? t[cell] : kind === STACK ? (hi > lo ? st[hi - 1] : 0) : ns[at];
        let push = -1;
        switch (ops[at]) {
          case 0:
            t[cell] = (t[cell] + v) & m;
            break;
          case 1:
            t[cell] = (t[cell] - v) & m;
            break;
          case 2:
            cell += v;
            if (cell >= TAPE) cell %= TAPE;
            break;
          case 3:
            cell -= v;
            if (cell < 0) cell = ((cell % TAPE) + TAPE) % TAPE;
            break;
          case 4:
            if (v >= TAPE)
              this.fail(
                this.program[at].file,
                this.program[at].line,
                `cell ${v} is outside the tape`,
              );
            cell = v;
            break;
          case 5:
            for (let j = 0; j < v; j++) out.push(t[(cell + j) % TAPE] & 0xff);
            break;
          case 6:
            for (let j = 0; j < v; j++) {
              const digits = (j > 0 ? ' ' : '') + t[(cell + j) % TAPE];
              for (let d = 0; d < digits.length; d++) out.push(digits.charCodeAt(d));
            }
            break;
          case 7:
            t[cell] = this.read < this.input.length ? this.input[this.read++] : 0;
            break;
          case 8:
            if (t[cell] === 0) at = matches[at];
            break;
          case 9:
            if (t[cell] !== 0) at = matches[at];
            break;
          case 10:
            if ((kind === NONE ? t[cell] : v) === 0) at = matches[at];
            break;
          case 11:
            if (v !== t[cell]) at = matches[at];
            break;
          case 12:
            break;
          case 13:
            at = end - 1;
            break;
          case 14:
            push = t[cell];
            break;
          case 15:
            lo = hi = 0;
            break;
          case 16:
            if (hi > lo) lo++;
            break;
          case 17:
            t[cell] = hi > lo ? st[hi - 1] : 0;
            break;
          case 18:
          case 19:
          case 20:
          case 21:
          case 22:
            push = this.combined(at, hi > lo ? st[hi - 1] : 0, hi - lo > 1 ? st[hi - 2] : 0);
            break;
          case 23:
            at = matches[at];
            break;
          case 24:
            if (!(t[cell] < v)) at = matches[at];
            break;
          case 25:
            if (!(t[cell] > v)) at = matches[at];
            break;
        }
        if (push >= 0) {
          if (hi === st.length) {
            const bigger = new Float64Array(Math.max(1024, (hi - lo) * 2));
            bigger.set(st.subarray(lo, hi));
            hi -= lo;
            lo = 0;
            st = this.stack = bigger;
          }
          st[hi++] = push;
        }
        at++;
      }
    } finally {
      this.at = at;
      this.cell = cell;
      this.lo = lo;
      this.hi = hi;
      this.done = at >= end;
    }
    return k;
  }

  combined(at, top, second) {
    const op = this.ops[at],
      m = this.mask;
    if (op === 18) return ((top + second) & m) >>> 0;
    if (op === 19) return ((top - second) & m) >>> 0;
    if (op === 20) return (Math.imul(top, second) & m) >>> 0;
    if (second === 0)
      this.fail(
        this.program[at].file,
        this.program[at].line,
        `${OPS[op]} by 0: the second value on the stack is 0`,
      );
    return op === 21 ? Math.floor(top / second) : top % second;
  }

  // the top first, the way the page shows them
  tops(n) {
    return Array.from(this.stack.subarray(Math.max(this.lo, this.hi - n), this.hi)).reverse();
  }

  take() {
    if (!this.out.length) return null;
    const bytes = new Uint8Array(this.out);
    this.out.length = 0;
    return bytes;
  }
}
