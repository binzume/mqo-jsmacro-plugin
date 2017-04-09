"use strict";

// pa: 圧力角
// m: モジュール
// n: 歯数
// r: 基準円半径
// m,n or m,r を指定
function mkParam(opt) {
	let pa = opt['pa'] || 20;
	let m = opt['m'] || 2;
	let n = opt['n'] || Math.floor(opt['r']*2 / m);
	let r = opt['r'] || m * n / 2;

	return {m: m, n: n, r: r, pa: pa};
}

// gear
function mkTeeth(param) {
	let n = param.n;
	let m = param.m;
	let r = param.r;
	let deg = param.pa;
	let r1 = r + m * 1.0;
	let r2 = r - m * 1.25;
	let rb = m * n / 2 * Math.cos(deg * Math.PI / 180);

	let cur = [[r2,0]];
	let a1 = 0;
	for (let t = 0; t < 1.0; t += 0.05) {
		let x = rb * (Math.cos(t) + t * Math.sin(t));
		let y = rb * (Math.sin(t) - t * Math.cos(t));
		if (x*x + y*y > r*r && a1 == 0) {
			let l = cur[cur.length - 1];
			let l1 = Math.sqrt(l[0]*l[0] + l[1]*l[1]);
			let l2 = Math.sqrt(x*x + y*y);
			let px = l[0] + (x - l[0]) * (r-l1) / (l2-l1);
			let py = l[1] + (y - l[1]) * (r-l1) / (l2-l1);
			cur.push([px, py]);
			a1 = -Math.atan2(py,px) - Math.PI * 2 / n / 4;
			//cur.push([r, 0]);
		}
		if (x*x + y*y > r1*r1) {
			let l = cur[cur.length - 1];
			let l1 = Math.sqrt(l[0]*l[0] + l[1]*l[1]);
			let l2 = Math.sqrt(x*x + y*y);
			let px = l[0] + (x - l[0]) * (r1-l1) / (l2-l1);
			let py = l[1] + (y - l[1]) * (r1-l1) / (l2-l1);
			cur.push([px, py]);
			break;
		}
		cur.push([x, y]);
	}
	//cur.push([r1,0]);

	let ret = [];
	cur.forEach( (p,i) => {
		let px = p[0]*Math.cos(a1) - p[1] * Math.sin(a1);
		let py = p[0]*Math.sin(a1) + p[1] * Math.cos(a1);
		ret.push([px, py]);
	});
	cur.reverse().forEach( (p,i) => {
		let px = p[0]*Math.cos(a1) - p[1] * Math.sin(a1);
		let py = p[0]*Math.sin(a1) + p[1] * Math.cos(a1);
		ret.push([px, -py]);
	});
	return ret;
}

function involuteGear(dst, m,n,r, opt) {
	let v0 = dst.verts.append(0, 0, 0);
	let a = Math.PI*2 / n;
	let param = mkParam({m: m, n:n, r:r});

	for (let i=0; i<n; i++) {
		let vs = mkTeeth(param).map( (p) => {
			let px = p[0]*Math.cos(a*i) - p[1] * Math.sin(a*i);
			let py = p[0]*Math.sin(a*i) + p[1] * Math.cos(a*i);
			return dst.verts.append(px, 0, py);
		});
		dst.faces.append(vs);
		dst.faces.append([v0, vs[0], vs[vs.length-1]]);

		let p = mkTeeth(param)[0];
		let px = p[0]*Math.cos(a*(i+1)) - p[1] * Math.sin(a*(i+1));
		let py = p[0]*Math.sin(a*(i+1)) + p[1] * Math.cos(a*(i+1));
		let vn = dst.verts.append(px, 0, py);
		dst.faces.append([v0, vs[vs.length-1], vn]);
	}
	return param;
}

if (typeof exports === "undefined") {
	// test
	let dst = document.objects[0];
	involuteGear(dst, 2, 10);
} else {
	exports["involuteGear"] = involuteGear;
}
