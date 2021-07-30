import * as _dialog from "mqwidget";
import * as fs from "fs";

// geometry
class Vector3 {
	constructor(x = 0, y = 0, z = 0) {
		if (x.x !== undefined) {
			this.x = x.x; this.y = x.y; this.z = x.z;
		} else if (x.length === 3) {
			this.x = x[0]; this.y = x[1]; this.z = x[2];
		} else {
			this.x = x; this.y = y; this.z = z;
		}
	}
	plus(v) { return new Vector3(this.x + v.x, this.y + v.y, this.z + v.z); }
	minus(v) { return new Vector3(this.x - v.x, this.y - v.y, this.z - v.z); }
	mul(a) { return new Vector3(this.x * a, this.y * a, this.z * a); }
	div(a) { return new Vector3(this.x / a, this.y / a, this.z / a); }
	negated() { return new Vector3(-this.x, -this.y, -this.z); }
	dot(v) { return this.x * v.x + this.y * v.y + this.z * v.z; }
	cross(v) {
		return new Vector3(
			this.y * v.z - this.z * v.y,
			this.z * v.x - this.x * v.z,
			this.x * v.y - this.y * v.x);
	}
	length() { return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z); }
	distanceTo(v) { return Math.sqrt(this.distanceToSqr(v)); }
	distanceToSqr(v) {
		let dx = this.x - v.x, dy = this.y - v.y, dz = this.z - v.z;
		return dx * dx + dy * dy + dz * dz;
	}
	unit() { return this.div(this.length()); }
	lerp(v, t) { return this.plus(v.minus(this).mul(t)); }
	clone() { return new Vector3(this.x, this.y, this.z); }
	toString() { return "(" + this.x + " " + this.y + " " + this.z + ")"; }
}

class Quaternion {
	constructor(x = 0, y = 0, z = 0, w = 1) {
		this.x = x; this.y = y; this.z = z; this.w = w;
	}
	length() { return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w); }
	dot(v) { return this.x * v.x + this.y * v.y + this.z * v.z + this.w * v.w; }
	multiply(q) { return Quaternion.multiply(this, q); }
	normalize() {
		let l = this.length();
		if (l > 0) {
			this.x /= l;
			this.y /= l;
			this.z /= l;
			this.w /= l;
		} else {
			this.w = 1;
		}
	}
	applyTo(v) {
		let vx = v.x, vy = v.y, vz = v.z;
		let x = this.x, y = this.y, z = this.z, w = this.w;
		let ix = w * vx + y * vz - z * vy;
		let iy = w * vy + z * vx - x * vz;
		let iz = w * vz + x * vy - y * vx;
		let iw = - x * vx - y * vy - z * vz;
		return new Vector3(
			ix * w + iw * - x + iy * - z - iz * - y,
			iy * w + iw * - y + iz * - x - ix * - z,
			iz * w + iw * - z + ix * - y - iy * - x
		);
	}
	clone() { return new Quaternion(this.x, this.y, this.z); }
	toString() { return "(" + this.x + " " + this.y + " " + this.z + " " + this.w + ")"; }
	static multiply(a, b) {
		return new Quaternion(
			a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, // i
			a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, // j
			a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, // k
			a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, // 1
		);
	}
	static fromAxisAngle(v, rad) {
		let s = Math.sin(rad / 2);
		return new Quaternion(
			v.x * s, v.y * s, v.z * s, Math.cos(rad / 2)
		);
	}
}

class Matrix4 {
	constructor(arr) {
		this.m = new Float32Array(arr || [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
	}
	mul(m) { return Matrix4.multiply(this, m, new Matrix4()); }
	applyTo(p) {
		let m = this.m;
		// assert(w == 1);
		if (p.length > 0) {
			return [
				m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3] + m[12],
				m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7] + m[13],
				m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]] + m[14];
		} else {
			return new Vector3(
				m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3] + m[12],
				m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7] + m[13],
				m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11] + m[14]);
		}
	}
	transformV(p) { return this.applyTo(p); }
	transform(p) { return this.applyTo(p); }
	toString() {
		return 'M(' + this.m.join() + ')';
	}
	static scaleMatrix(x, y, z) {
		return new Matrix4([x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1]);
	}
	static translateMatrix(x, y, z) {
		return new Matrix4([1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1]);
	}
	static rotateMatrix(x, y, z, a) {
		a *= Math.PI / 180;
		const c = Math.cos(a), s = Math.sin(a);
		const c1 = 1 - c;
		return new Matrix4([
			x * x * c1 + c, x * y * c1 - z * s, x * z * c1 + y * s, 0,
			y * x * c1 + z * s, y * y * c1 + c, y * z * c1 - x * s, 0,
			z * x * c1 - y * s, z * y * c1 + x * s, z * z * c1 + c, 0,
			0, 0, 0, 1
		]);
	}
	static multiply(m1, m2, result) {
		let a = m1.m, b = m2.m, r = result.m;
		r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
		r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
		r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
		r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
		r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
		r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
		r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
		r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
		r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
		r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
		r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
		r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
		r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
		r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
		r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
		r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
		return result;
	}
}

class Plane {
	constructor(normal, w) {
		this._normal = normal;
		this._w = w;
	}
	static fromPoints(a, b, c) {
		let n = b.minus(a).cross(c.minus(a)).unit();
		return new Plane(n, n.dot(a));
	}
	get normal() { return this._normal; }
	get w() { return this._w; }
	flipped() { return new Plane(this._normal.negated(), -this._w); }
	toString() { return "Plane(" + this._normal.toString() + "," + this + ")"; }
}

process.defineModule('geom', [
	{ name: 'Vector3', value: Vector3 },
	{ name: 'Quaternion', value: Quaternion },
	{ name: 'Matrix4', value: Matrix4 },
	{ name: 'Plane', value: Plane },
]);

// dialog TODO
let dialog = {
	alertDialog: _dialog.alertDialog,
	fileDialog: _dialog.fileDialog,
	folderDialog: _dialog.folderDialog,
	modalDialog: _dialog.modalDialog,
	confirmDialog: function (msg) { let r = _dialog.modalDialog({ title: msg, items: [msg] }, ["OK", "Cancel"]); return r && r.button == 0; },
	promptDialog: function (msg, v) { let r = _dialog.modalDialog({ title: msg, items: [{ type: "text", value: v || "", id: "_" }] }, ["OK"]); return r && r.values['_']; }
};

// module
globalThis.module = (function () {
	let geom = {
		Vector3: Vector3,
		Vertex: Vector3,
		Plane: Plane,
		Matrix: Matrix4
	};
	let modules = {};
	modules['dialog'] = { exports: dialog, loaded: true };
	modules['geom'] = { exports: geom, loaded: true };
	// modules['fs'] = {exports: fs, loaded: true};
	// modules['child_process'] = {exports: child_process, loaded: true};
	return {
		require: function (name) {
			if (!modules[name]) {
				let script = fs.readFile(process.scriptDir() + name);
				let mod = {
					id: name,
					filename: name,
					exports: {},
					require: module.require,
					loaded: true
				};
				let init = process.execScript("(function(exports, require, module, __filename){" + script + "\n})", name);
				init(mod.exports, mod.require, mod, name);
				modules[name] = mod;
			}
			return modules[name].exports;
		},
		include: function (name) {
			return process.load(name, false);
		}
	};
})();
globalThis.require = module.require;  // deprecated

// Built-in functions
globalThis.alert = dialog.alertDialog;
globalThis.confirm = dialog.confirmDialog;
globalThis.prompt = dialog.promptDialog;
globalThis.console = {
	log(...v) { process.stdout.write(v.join(" ")); },
	error(...v) { process.stderr.write(v.join(" ")); },
};

let timerId = 0;
globalThis.setTimeout = function (f, timeout) {
	timerId++;
	let args = arguments.length > 2 ? [arguments[2]] : [];
	let wf = function () {
		f.apply(null, args);
	};
	process.registerTimer(wf, timeout, timerId);
	return timerId;
}

globalThis.setInterval = function (f, interval) {
	timerId++;
	let tid = timerId;
	let args = arguments.length > 2 ? [arguments[2]] : [];
	let wf = function () {
		f.apply(null, args);
		process.registerTimer(wf, interval, tid);
	};
	process.registerTimer(wf, interval, tid);
	return tid;
}

globalThis.clearTimeout = globalThis.clearInterval = function (id) {
	process.removeTimer(id);
};

let listeners = {};

process.onmessage = function (msg) {
	if (listeners[msg]) {
		for (let l of listeners[msg]) {
			l[1]({ document: l[0], event: msg });
		}
	}
	if (msg === '_dispose') {
		// avoid listener leak.
		delete MQDocument.prototype.addEventListener;
		delete process.onmessage;
	}
};

MQDocument.prototype.getObjectByName = function (name) {
	return this.objects.find(o => o && o.name == name);
};

MQDocument.prototype.addEventListener = function (name, f) {
	listeners[name] = listeners[name] || [];
	listeners[name].push([this, f]);
};

MQObject.prototype.applyTransform = function (tr) {
	const length = this.verts.length;
	if (typeof tr === "function") {
		for (let i = 0; i < length; i++) {
			this.verts[i] = tr(this.verts[i]);
		}
	} else {
		for (let i = 0; i < length; i++) {
			this.verts[i] = tr.applyTo(this.verts[i]);
		}
	}
};

MQObject.prototype.getChildren = function (recursive = false) {
	if (this.index < 0) return [];
	let objects = mqdocument.objects;
	let children = [];
	let d = this.depth + 1;
	for (let i = this.index + 1; i < objects.length; i++) {
		if (!objects[i]) {
			continue;
		} else if (objects[i].depth < d) {
			break;
		} else if (recursive || objects[i].depth == d) {
			children.push(objects[i]);
		}
	}
	return children;
};

Object.defineProperty(MQObject.prototype, 'globalMatrix', {
	get: function () {
		// TODO: return new Matrix()
		return document.getGlobalMatrix(this);
	}
});

// deprecated
globalThis.document = mqdocument;
globalThis.MQMatrix = Matrix4;
