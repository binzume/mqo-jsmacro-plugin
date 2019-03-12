"use strict";

// modules
(function (global){

global.mqdocument = document;

// geometry
class Vector3 {
	constructor(x,y,z) {
		if (x.x !== undefined) {
			this.x = x.x; this.y = x.y; this.z = x.z;
		} else if (x.length === 3) {
			this.x = x[0]; this.y = x[1]; this.z = x[2];
		} else {
			this.x = x; this.y = y; this.z = z;
		}
	}
	length() { return Math.sqrt(this.x*this.x + this.y*this.y + this.z*this.z); }
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
			this.x * v.y - this.y * v.x );
	}
	unit() { return this.div(this.length()); }
	lerp(v, t) { return this.plus(v.minus(this).mul(t)); }
	toString() { return "(" + this.x + " " + this.y + " " + this.z + ")";}
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

class Matrix{
	constructor(arr) {
		this.m = new Float32Array(arr || [1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1]);
	}
	mul(m) { return Matrix.multiply(this, m); }
	transform(p) {
		let m = this.m;
		if (p.length > 0) {
			return [
				m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3],
				m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
				m[8] * p[0] + m[9] * p[1] + m[10]* p[2] + m[11]];
		} else {
			return new Vector3(
				m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3],
				m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
				m[8] * p.x + m[9] * p.y + m[10]* p.z + m[11]);
		}
	}
	static scaleMatrix(x,y,z) {
		return new Matrix([x, 0, 0, 0,  0, y, 0, 0,  0, 0, z, 0,  0, 0, 0, 1]);
	}
	static translateMatrix(x,y,z) {
		return new Matrix([1, 0, 0, x,  0, 1, 0, y,  0, 0, 1, z,  0, 0, 0, 1]);
	}
	static rotateMatrix(x,y,z,a) {
		a *= Math.PI / 180;
		const c = Math.cos(a), s = Math.sin(a);
		const c1 = 1 - c;
		return new Matrix([
			x * x * c1 + c,  x * y * c1 - z * s,  x * z * c1 + y * s,  0,
			y * x * c1 + z * s,  y * y * c1 + c,  y * z * c1 - x * s,  0,
			z * x * c1 - y * s,  z * y * c1 + x * s,  z * z * c1 + c,  0,
			0, 0, 0, 1
		]);
	}
}
Matrix.multiply = function(m1, m2) {
	let result = new Matrix();
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
};

let geom = {
	Vector3: Vector3,
	Vertex: Vector3,
	Plane: Plane,
	Matrix: Matrix
};

// dialog TODO
let dialog = {
	alertDialog: process._dialog.alertDialog,
	fileDialog: process._dialog.fileDialog,
	folderDialog: process._dialog.folderDialog,
	modalDialog: process._dialog.modalDialog,
	confirmDialog: function(msg) { let r = process._dialog.modalDialog({title:msg, items:[msg]}, ["OK", "Cancel"]); return r && r.button == 0; },
	promptDialog: function(msg, v) { let r = process._dialog.modalDialog({title:msg, items:[{type:"text", value:v||"", id:"_"}]}, ["OK"]); return r && r.values['_']; }
};

// module
global.module = (function(unsafe){
	let modules = {};
	modules['dialog'] = {exports: dialog, loaded: true};
	modules['geom'] = {exports: geom, loaded: true};
	// modules['fs'] = {exports: unsafe.fs, loaded: true};
	// modules['child_process'] = {exports: unsafe.child_process, loaded: true};
	return {
		require: function(name) {
			if (!modules[name]) {
				console.log("load: "+name);
				let script = unsafe.fs.readFile(process.scriptDir() + "/" + name);
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
		include: function(name) {
			console.log("load: "+name);
			let script = unsafe.fs.readFile(process.scriptDir() + "/" + name);
			return process.execScript(script, name);
		}
	};
})(unsafe); // global.unsafe : only core.js.

global.require = global.module.require;

global.MQMatrix = Matrix;

})(this);


// Built-in functions
let alert = require("dialog").alertDialog; // alert(message);
let confirm = require("dialog").confirmDialog; // confirm(message) -> boolean
let prompt = require("dialog").promptDialog; // confirm(message, default) -> value
let console = {
	"log" : function(v) { process.stdout.write(v); }
};

function setTimeout(f, timeout) {
	let args = arguments.length > 2 ? [arguments[2]] : [];
	var wf = function(){
		f.apply(null, args);
	};
	process.nextTick(wf, interval);
}

function setInterval(f, interval) {
	let args = arguments.length > 2 ? [arguments[2]] : [];
	var wf = function(){
		f.apply(null, args);
		process.nextTick(wf, interval);
	};
	process.nextTick(wf, interval);
}


MQObject.prototype.transform = function(tr) {
	const length = this.verts.length;
	if (typeof tr === "function") {
		for(let i=0; i< length; i++) {
			this.verts[i] =  tr(this.verts[i]);
		}
	} else {
		for(let i=0; i< length; i++) {
			this.verts[i] = tr.transform(this.verts[i]);
		}
	}
};

