// Runs rucas.wasm in its own thread, with the program and input files held in memory.

const ENOENT = 44,
  EBADF = 8,
  ENOSYS = 52;
const PREOPEN = 3;
// a post per write would flood the page, so output goes out at most this often
const POST_EVERY = 50;
const TAPE_SIZE = 30000,
  WINDOW = 64;

class Exit {
  constructor(code) {
    this.code = code;
  }
}

let memory;
let where;
let tape = null;
let files;
let open = new Map();
let nextFd = PREOPEN + 1;
let args;
let pending = { 1: [], 2: [] };
let lastPost = 0;

const view = () => new DataView(memory.buffer);
const bytes = () => new Uint8Array(memory.buffer);
const text = (ptr, len) => new TextDecoder().decode(bytes().slice(ptr, ptr + len));

// read only while the program writes, since at exit its destructors have already freed the tape
function snapshot() {
  const v = view();
  const cells = v.getUint32(where.cells, true);
  const pointer = Number(v.getBigInt64(where.cell, true));
  const stackFrom = v.getUint32(where.stack, true),
    stackTo = v.getUint32(where.stack + 4, true);
  const start = Math.max(0, Math.min(pointer - WINDOW / 2, TAPE_SIZE - WINDOW));
  const stack = [];
  for (let at = stackTo - 4; at >= stackFrom && stack.length < 32; at -= 4)
    stack.push(v.getUint32(at, true));
  tape = {
    start,
    pointer,
    values: Array.from(new Uint32Array(memory.buffer, cells + start * 4, WINDOW)),
    depth: (stackTo - stackFrom) / 4,
    stack,
  };
}

function post(force) {
  const now = performance.now();
  if (!force && now - lastPost < POST_EVERY) return;
  lastPost = now;
  for (const fd of [1, 2]) {
    if (pending[fd].length) self.postMessage({ fd, chunks: pending[fd] });
    pending[fd] = [];
  }
  if (tape) self.postMessage({ tape });
  tape = null;
}

function readInto(source, iovs, count, readPtr) {
  const v = view();
  let total = 0;
  for (let k = 0; k < count; k++) {
    const ptr = v.getUint32(iovs + k * 8, true);
    const len = v.getUint32(iovs + k * 8 + 4, true);
    const part = source.data.subarray(source.at, source.at + len);
    bytes().set(part, ptr);
    source.at += part.length;
    total += part.length;
  }
  v.setUint32(readPtr, total, true);
  return 0;
}

const wasi = {
  args_sizes_get(countPtr, sizePtr) {
    view().setUint32(countPtr, args.length, true);
    view().setUint32(
      sizePtr,
      args.reduce((n, a) => n + a.length + 1, 0),
      true,
    );
    return 0;
  },
  args_get(argvPtr, bufPtr) {
    args.forEach((a, k) => {
      view().setUint32(argvPtr + k * 4, bufPtr, true);
      bytes().set(a, bufPtr);
      bytes()[bufPtr + a.length] = 0;
      bufPtr += a.length + 1;
    });
    return 0;
  },
  environ_sizes_get(countPtr, sizePtr) {
    view().setUint32(countPtr, 0, true);
    view().setUint32(sizePtr, 0, true);
    return 0;
  },
  environ_get() {
    return 0;
  },
  // one folder, "/", so fopen finds the files by their plain names
  fd_prestat_get(fd, ptr) {
    if (fd !== PREOPEN) return EBADF;
    view().setUint8(ptr, 0);
    view().setUint32(ptr + 4, 1, true);
    return 0;
  },
  fd_prestat_dir_name(fd, ptr) {
    bytes()[ptr] = 47;
    return 0;
  },
  fd_fdstat_get(fd, ptr) {
    const v = view();
    v.setUint8(ptr, fd < 3 ? 2 : fd === PREOPEN ? 3 : 4);
    v.setUint16(ptr + 2, 0, true);
    v.setBigUint64(ptr + 8, 0xffffffffffffffffn, true);
    v.setBigUint64(ptr + 16, 0xffffffffffffffffn, true);
    return 0;
  },
  path_open(dirFd, dirFlags, pathPtr, pathLen, oflags, rights, inherited, fdFlags, fdPtr) {
    const name = text(pathPtr, pathLen).replace(/^\.?\//, '');
    if (!files.has(name)) return ENOENT;
    const fd = nextFd++;
    open.set(fd, { data: files.get(name), at: 0 });
    view().setUint32(fdPtr, fd, true);
    return 0;
  },
  fd_read(fd, iovs, count, readPtr) {
    if (fd === 0) return readInto({ data: new Uint8Array(0), at: 0 }, iovs, count, readPtr);
    if (!open.has(fd)) return EBADF;
    return readInto(open.get(fd), iovs, count, readPtr);
  },
  fd_write(fd, iovs, count, writtenPtr) {
    const v = view();
    let total = 0;
    for (let k = 0; k < count; k++) {
      const ptr = v.getUint32(iovs + k * 8, true);
      const len = v.getUint32(iovs + k * 8 + 4, true);
      if (fd === 1 || fd === 2) pending[fd].push(bytes().slice(ptr, ptr + len));
      total += len;
    }
    v.setUint32(writtenPtr, total, true);
    snapshot();
    post(false);
    return 0;
  },
  fd_seek(fd, offset, whence, newPtr) {
    const file = open.get(fd);
    if (!file) return EBADF;
    const from = [0, file.at, file.data.length][whence];
    file.at = Math.max(0, Math.min(file.data.length, from + Number(offset)));
    view().setBigUint64(newPtr, BigInt(file.at), true);
    return 0;
  },
  fd_close(fd) {
    open.delete(fd);
    return 0;
  },
  proc_exit(code) {
    throw new Exit(code);
  },
};

// the interpreter needs nothing else, so any other call gets a plain "not supported"
const imports = {
  wasi_snapshot_preview1: new Proxy(wasi, { get: (t, name) => t[name] || (() => ENOSYS) }),
};

self.onmessage = async ({ data }) => {
  const encode = (s) => new TextEncoder().encode(s);
  files = new Map(data.files.map((f) => [f.name, encode(f.text)]));
  args = ['rucas', ...data.files.map((f) => f.name)];
  if (data.input) {
    files.set('input', encode(data.input));
    args.push('input');
  }
  args = args.map(encode);
  const instance = await WebAssembly.instantiate(data.module, imports);
  memory = instance.exports.memory;
  where = {
    cells: instance.exports.m_cells.value,
    cell: instance.exports.m_cell.value,
    stack: instance.exports.m_stack.value,
  };
  let code = 0;
  try {
    instance.exports._start();
  } catch (e) {
    if (!(e instanceof Exit)) throw e;
    code = e.code;
  }
  post(true);
  self.postMessage({ done: code });
};
