"use strict";

var console = {
	"log" : function(v) { process.stdout.write(v); }
};

MQObject.prototype.transform = function(tr) {
	var length = this.verts.length;
	if (typeof tr === "function") {
		for(var i=0; i< length; i++) {
			this.verts[i] =  tr(this.verts[i]);
		}
	} else {
		for(var i=0; i< length; i++) {
			var p = this.verts[i];
			var m = tr.m;
			this.verts[i] = [
				m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3],
				m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
				m[8] * p[0] + m[9] * p[1] + m[10]* p[2] + m[11]
			];
		}
	}
};

function MQMatrix(arr) {
	this.m = new Float32Array(arr || [1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1]);
}

MQMatrix.prototype.mul = function(m) {
	return MQMatrix.multiply(this, m);
};

MQMatrix.scaleMatrix = function(x,y,z) {
	return new MQMatrix([x, 0, 0, 0,  0, y, 0, 0,  0, 0, z, 0,  0, 0, 0, 1]);
};

MQMatrix.translateMatrix = function(x,y,z) {
	return new MQMatrix([1, 0, 0, x,  0, 1, 0, y,  0, 0, 1, z,  0, 0, 0, 1]);
};

MQMatrix.rotateMatrix = function(x,y,z,a) {
	a *= Math.PI / 180;
	var c = Math.cos(a), s = Math.sin(a);
	var c1 = 1 - c;
	return new MQMatrix([
		x * x * c1 + c,  x * y * c1 - z * s,  x * z * c1 + y * s,  0,
		y * x * c1 + z * s,  y * y * c1 + c,  y * z * c1 - x * s,  0,
		z * x * c1 - y * s,  z * y * c1 + x * s,  z * z * c1 + c,  0,
		0, 0, 0, 1
	]);
};

MQMatrix.multiply = function(m1, m2) {
	var result = new MQMatrix();
	var a = m1.m, b = m2.m, r = result.m;
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
