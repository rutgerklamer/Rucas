// Loads the examples into the page and runs them in a worker that can be stopped.

const EXAMPLES = [
	["Samples", [
		["hello_world", ["samples/hello_world.ruc"]],
	]],
	["Code patterns", ["cells", "pointers", "numbers", "points", "loops", "lists", "chances", "functions", "recursion"]
		.map(name => [name, [`code_patterns/${name}.ruc`]])],
	["Showcase", [
		["names", ["showcase/names/names.ruc"], "showcase/names/names.txt"],
		["names_optimised", ["showcase/names/names_optimised.ruc"], "showcase/names/names.txt"],
		["lm_base test", ["showcase/lm_base/base.ruc", "showcase/lm_base/test.ruc"]],
		["names_network", ["showcase/lm_base/base.ruc", "showcase/names/names_network.ruc"], "showcase/names/names.txt"],
		["names_attention", ["showcase/lm_base/base.ruc", "showcase/names/names_attention.ruc"], "showcase/names/names.txt"],
		["shakespeare", ["showcase/lm_base/base.ruc", "showcase/shakespeare/shakespeare.ruc"], "showcase/shakespeare/shakespeare.txt"],
	]],
];
// a program that never ends never stops printing, so only the newest part stays
const KEEP = 100000;
const CELL_WIDTH = 56;
// instructions a second; the last runs rucas.wasm when a run starts there
const SPEEDS = [1, 2, 5, 10, 25, 100, 1000, 10000, 100000, 1e6, 1e7, 1e8, Infinity];
const EMPTY = { note: "Before the run", start: 0, pointer: 0, values: new Array(64).fill(0), depth: 0, stack: [] };
// a frame of stepping at full speed, so the page stays responsive
const FRAME = 12;
// instructions per call into the twin, small enough to check the time between them
const PIECE = 65536;

const $ = id => document.getElementById(id);
const picker = $("example"), program = $("program"), lines = $("lines"), input = $("input");
const runButton = $("run"), stepButton = $("step"), stopButton = $("stop"), speed = $("speed"), status = $("status"), error = $("error"), output = $("output");
const module = WebAssembly.compileStreaming(fetch("rucas.wasm"));
const fetchText = path => fetch(`../${path}`).then(r => r.ok ? r.text() : Promise.reject(new Error(`cannot load ${path}`)));
const base = path => path.split("/").pop();

let worker = null;
let machine = null;
let auto = false;
let timer = null;
let steps = 0;
let pace = { steps: 0, time: 0 };
let last = 0;
let decoder = null;
let before = [];
let name = "program.ruc";
let shown = new Map();
let lastTape = null;
let shownStack = new Map();

for (const [group, items] of EXAMPLES) {
	const optgroup = document.createElement("optgroup");
	optgroup.label = group;
	for (const [label, files, data] of items)
		optgroup.append(new Option(label, JSON.stringify([files, data || ""])));
	picker.append(optgroup);
}

function showLines() {
	const count = program.value.split("\n").length;
	lines.textContent = Array.from({ length: count }, (_, k) => k + 1).join("\n");
	lines.scrollTop = program.scrollTop;
}

async function load() {
	const [files, data] = JSON.parse(picker.value);
	stop();
	try {
		const texts = await Promise.all(files.map(fetchText));
		// the shared base is not the example itself, so it runs first but stays out of the editor
		before = files.slice(0, -1).map((path, k) => ({ name: base(path), text: texts[k] }));
		name = base(files[files.length - 1]);
		program.value = texts[texts.length - 1];
		input.value = data.endsWith(".txt") ? await fetchText(data) : data;
		$("also").hidden = before.length === 0;
		$("also").textContent = `Runs after ${before.map(f => f.name).join(", ")}, as one program.`;
		clear();
	}
	catch (e) {
		showError(`${e.message}. The page needs to be served over http, from the root of the repository.`);
	}
	program.scrollTop = 0;
	showLines();
}

const printable = value => value >= 32 && value < 127 ? String.fromCharCode(value) : "";

function makeCell(rows, className, title) {
	const cell = document.createElement("div");
	cell.className = className;
	cell.title = title || "";
	for (const [tag, text] of [["small", rows[0]], ["b", rows[1]], ["i", rows[2] || "\u00a0"]]) {
		const row = document.createElement(tag);
		row.textContent = text;
		cell.append(row);
	}
	return cell;
}

function showTape(tape) {
	lastTape = tape;
	const box = $("tape");
	const count = Math.max(4, Math.floor(box.clientWidth / CELL_WIDTH) - 1);
	const first = Math.max(tape.start, Math.min(tape.pointer - Math.floor(count / 2), tape.start + tape.values.length - count));
	const value = tape.values[tape.pointer - tape.start];
	const letter = printable(value);
	const [top, ...below] = tape.stack.slice(0, 4);
	const stack = tape.depth
		? `the stack is ${tape.depth} deep, ${top} on top${below.length ? `, then ${below.join(", ")}` : ""}${tape.depth > 4 ? ` and ${tape.depth - 4} more` : ""}`
		: "the stack is empty";
	$("summary").textContent = `${tape.note}; the pointer is at cell ${tape.pointer}, holding ${value}${letter ? ` (${letter})` : ""}, and ${stack}.`;
	$("summary").title = $("summary").textContent;
	// the first column names the three rows, so the strip explains itself without a line of text
	const cells = [makeCell(["cell", "value", "char"], "cell names")];
	for (let k = first; k < first + count; k++) {
		const v = tape.values[k - tape.start];
		const lit = shown.has(k) && shown.get(k) !== v;
		cells.push(makeCell([k, v, printable(v)], "cell" + (k === tape.pointer ? " here" : "") + (lit ? " changed" : ""), `cell ${k}; ${v}`));
	}
	box.replaceChildren(...cells);
	shown = new Map(tape.values.map((v, k) => [tape.start + k, v]));
	showStack(tape, count);
}

// a value lights up by its height from the bottom, since counted from the top one push would light them all
function showStack(tape, count) {
	const cells = [makeCell(["from top", "value", "char"], "cell names")];
	const fits = tape.depth > count ? count - 1 : tape.depth;
	for (let i = 0; i < fits; i++) {
		const v = tape.stack[i], height = tape.depth - 1 - i;
		const lit = shownStack.has(height) && shownStack.get(height) !== v;
		cells.push(makeCell([i + 1, v, printable(v)], "cell" + (lit ? " changed" : ""), `${i + 1} from the top; ${v}`));
	}
	if (tape.depth > fits)
		cells.push(makeCell([`+${tape.depth - fits}`, "…", ""], "cell", `${tape.depth - fits} more below`));
	if (!tape.depth)
		cells.push(makeCell(["", "empty", ""], "cell empty"));
	// spacers keep a stack cell as wide as a tape cell, so the two strips line up
	while (tape.depth && cells.length < count + 1)
		cells.push(makeCell(["", "", ""], "cell spacer"));
	$("stack").replaceChildren(...cells);
	shownStack = new Map(tape.stack.slice(0, fits).map((v, i) => [tape.depth - 1 - i, v]));
}

function show(text) {
	if (!text)
		return;
	output.textContent = (output.textContent + text).slice(-KEEP);
	output.scrollTop = output.scrollHeight;
}

function goToLine(line) {
	const starts = [0];
	for (let k = 0; k < program.value.length; k++)
		if (program.value[k] === "\n")
			starts.push(k + 1);
	const end = line < starts.length ? starts[line] - 1 : program.value.length;
	program.focus();
	program.setSelectionRange(starts[line - 1] ?? 0, end);
	program.scrollTop = (line - 3) * parseFloat(getComputedStyle(program).lineHeight);
	showLines();
}

function showError(text) {
	error.textContent = text.trim();
	const found = /(?:(\S+\.ruc) )?line (\d+)/.exec(text);
	if (!found || (found[1] && found[1] !== name))
		return fitError();
	const line = Number(found[2]);
	const button = document.createElement("button");
	button.type = "button";
	button.textContent = `Go to line ${line}`;
	button.onclick = () => goToLine(line);
	error.append(button);
	fitError();
}

// room under the output for the error, so it covers none of what was printed
function fitError() {
	output.style.paddingBottom = error.textContent ? `${error.offsetHeight + 8}px` : "";
	output.scrollTop = output.scrollHeight;
}

function showSpeed() {
	const s = SPEEDS[speed.value];
	const text = s === Infinity ? "Full" : `${s.toLocaleString("en")} a second`;
	$("speed-text").textContent = text;
	speed.setAttribute("aria-valuetext", text);
}

function setState() {
	runButton.disabled = Boolean(worker || auto);
	runButton.textContent = machine && !auto ? "Continue" : "Run";
	stepButton.disabled = Boolean(worker);
	stopButton.disabled = !(worker || machine);
	program.readOnly = Boolean(machine);
	placeCurrent();
}

function clear() {
	output.textContent = "";
	error.textContent = "";
	fitError();
	status.textContent = "";
	$("timer").textContent = "";
	shown = new Map();
	shownStack = new Map();
	showTape(EMPTY);
}

function files() {
	return [...before, { name, text: program.value }];
}

// base.ruc is not in the editor, so its lines get no bar
function placeCurrent() {
	const bar = $("current");
	const ins = machine && !machine.done ? machine.program[machine.at] : null;
	bar.hidden = !ins || ins.file !== machine.names.length - 1;
	if (bar.hidden)
		return;
	const style = getComputedStyle(program);
	const height = parseFloat(style.lineHeight);
	bar.style.height = `${height}px`;
	bar.style.top = `${parseFloat(style.paddingTop) + (ins.line - 1) * height - program.scrollTop}px`;
}

function followCurrent() {
	const ins = machine.program[machine.at];
	if (!ins || ins.file !== machine.names.length - 1)
		return;
	const height = parseFloat(getComputedStyle(program).lineHeight);
	const y = (ins.line - 1) * height;
	if (y < program.scrollTop || y + 2 * height > program.scrollTop + program.clientHeight)
		program.scrollTop = y - program.clientHeight / 3;
	lines.scrollTop = program.scrollTop;
}

function describe() {
	const ins = machine.program[machine.at];
	return `${ins.op}${ins.text ? ` ${ins.text}` : ""} on ${machine.where(ins.file, ins.line)} runs next`;
}

function machineTape() {
	const start = Math.max(0, Math.min(machine.cell - 32, machine.tape.length - 64));
	return {
		start,
		pointer: machine.cell,
		values: Array.from(machine.tape.subarray(start, start + 64)),
		depth: machine.hi - machine.lo,
		stack: machine.tops(32),
	};
}

function draw() {
	followCurrent();
	placeCurrent();
	const counted = `${steps.toLocaleString("en")} step${steps === 1 ? "" : "s"}`;
	showTape({ note: steps ? `After ${counted}` : "Before the run", ...machineTape() });
	$("timer").textContent = `${counted}${auto && SPEEDS[speed.value] > 25 ? `, ${rate()} a second` : ""}`;
}

// at the top of the slider the browser sets the pace, so the page says what it really reaches
function rate() {
	const seconds = (performance.now() - pace.time) / 1000;
	const n = seconds > 0 ? (steps - pace.steps) / seconds : 0;
	return n >= 1e6 ? `${Math.round(n / 1e6)} million` : Math.round(n).toLocaleString("en");
}

function end(text) {
	machine = null;
	auto = false;
	clearTimeout(timer);
	status.textContent = text;
	$("timer").textContent = "";
	setState();
}

function startMachine() {
	stop();
	clear();
	decoder = new TextDecoder();
	steps = 0;
	try {
		machine = new Machine(files(), input.value);
	}
	catch (e) {
		if (!(e instanceof RucasError))
			throw e;
		showError(e.message);
		end("Stopped with an error");
		return false;
	}
	if (machine.done) {
		end("Finished after 0 steps");
		return false;
	}
	draw();
	return true;
}

function runPieces(count, until) {
	while (count > 0 && !machine.done) {
		const ran = machine.run(Math.min(count, PIECE));
		steps += ran;
		count -= ran;
		if (until && performance.now() > until)
			return;
	}
}

function advance(count, until) {
	let failed = null;
	try {
		runPieces(count, until);
	}
	catch (e) {
		if (!(e instanceof RucasError))
			throw e;
		failed = e.message;
	}
	const out = machine.take();
	show(out ? decoder.decode(out, { stream: true }) : "");
	draw();
	if (failed) {
		showError(failed);
		end("Stopped with an error");
	}
	else if (machine.done)
		end(`Finished after ${steps.toLocaleString("en")} steps`);
}

function tick() {
	if (!machine || !auto)
		return;
	const s = SPEEDS[speed.value];
	if (s <= 25) {
		advance(1);
		if (machine && auto)
			timer = setTimeout(tick, 1000 / s);
	}
	// as many steps as the time since the last tick is worth, so a slow or hidden page keeps its speed
	else {
		const now = performance.now();
		const due = s === Infinity ? Infinity : Math.floor(s * (now - last) / 1000);
		const from = steps;
		if (due > 0)
			advance(due, now + FRAME);
		// a part of a step carries over to the next tick, a backlog the browser could not run does not
		last = steps - from < due ? now : last + due * 1000 / s;
		if (machine && auto)
			timer = setTimeout(tick, 4);
	}
}

function run() {
	if (!machine) {
		if (SPEEDS[speed.value] === Infinity)
			return runFull();
		if (!startMachine())
			return;
	}
	auto = true;
	last = performance.now();
	pace = { steps, time: last };
	status.textContent = "Running";
	setState();
	tick();
}

// the first Step only shows what runs first, so nothing happens unseen
function step() {
	if (!machine) {
		if (startMachine()) {
			status.textContent = describe();
			setState();
		}
		return;
	}
	auto = false;
	clearTimeout(timer);
	advance(1);
	if (machine)
		status.textContent = describe();
	setState();
}

async function runFull() {
	stop();
	clear();
	const compiled = await module;
	status.textContent = "Running";
	const started = performance.now();
	const ticking = setInterval(() => { $("timer").textContent = `${Math.round((performance.now() - started) / 1000)} s`; }, 1000);
	worker = new Worker("worker.js");
	worker.stop = (text = "Stopped") => {
		clearInterval(ticking);
		worker.terminate();
		worker = null;
		status.textContent = text;
		$("timer").textContent = "";
		setState();
	};
	worker.onmessage = listen(started);
	worker.onerror = e => {
		worker.stop("The interpreter crashed");
		showError(e.message);
	};
	setState();
	worker.postMessage({ module: compiled, files: files(), input: input.value });
}

// one decoder per stream, so a character split across two posts still comes out whole
function listen(started) {
	const out = new TextDecoder(), err = new TextDecoder();
	let errors = "";
	return ({ data }) => {
		if (data.tape)
			showTape({ note: "At the last output", ...data.tape });
		else if (data.fd === 1)
			show(data.chunks.map(c => out.decode(c, { stream: true })).join(""));
		else if (data.fd === 2)
			errors += data.chunks.map(c => err.decode(c, { stream: true })).join("");
		else {
			worker.stop(data.done === 0 ? `Finished in ${((performance.now() - started) / 1000).toFixed(1)} s` : "Stopped with an error");
			if (errors)
				showError(errors);
		}
	};
}

function stop() {
	if (worker)
		worker.stop();
	else if (machine)
		end("Stopped");
}

window.addEventListener("resize", () => { showTape(lastTape); placeCurrent(); });
program.addEventListener("input", showLines);
program.addEventListener("scroll", () => { lines.scrollTop = program.scrollTop; placeCurrent(); });
picker.addEventListener("change", load);
speed.addEventListener("input", () => { showSpeed(); pace = { steps, time: performance.now() }; });
runButton.addEventListener("click", run);
stepButton.addEventListener("click", step);
stopButton.addEventListener("click", stop);
showSpeed();
load();
