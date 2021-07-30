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
	let r2 = r - m * 1.2;
	let rb = m * n / 2 * Math.cos(deg * Math.PI / 180);

	let cur = [[r2,0]];
	let a1 = 0;
	for (let t = 0; t < 1.0; t += 0.02) {
		let x = rb * (Math.cos(t) + t * Math.sin(t));
		let y = rb * (Math.sin(t) - t * Math.cos(t));
		if (x*x + y*y < r2*r2) {
			continue;
		}
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
	let hole = 2;
	let hl = null;
	let tl = null;
	for (let i=0; i<n; i++) {
		let vs = mkTeeth(param).map( (p, idx) => {
			if (idx == 0 && tl) return tl;
			let px = p[0]*Math.cos(a*i) - p[1] * Math.sin(a*i);
			let py = p[0]*Math.sin(a*i) + p[1] * Math.cos(a*i);
			return dst.verts.append(px, 0, py);
		});
		dst.faces.append(vs);

		let p = mkTeeth(param)[0];
		let px = p[0]*Math.cos(a*(i+1)) - p[1] * Math.sin(a*(i+1));
		let py = p[0]*Math.sin(a*(i+1)) + p[1] * Math.cos(a*(i+1));
		let vn = dst.verts.append(px, 0, py);
		tl = vn;
		if (hole > 0) {
			let h1 = hl || dst.verts.append(Math.cos(a*i)*hole, 0, Math.sin(a*i)*hole);
			let h2 = dst.verts.append(Math.cos(a*(i+0.5))*hole, 0, Math.sin(a*(i+0.5))*hole);
			let h3 = dst.verts.append(Math.cos(a*(i+1))*hole, 0, Math.sin(a*(i+1))*hole);
			hl = h3;
			dst.faces.append([h2, h1,vs[0], vs[vs.length-1]]);
			dst.faces.append([h3, h2, vs[vs.length-1], vn]);
		} else {
			dst.faces.append([v0, vs[0], vs[vs.length-1]]);
			dst.faces.append([v0, vs[vs.length-1], vn]);
		}
	}
	return param;
}

if (typeof exports === "undefined") {
	let dialog = require('dialog');
	let content = {title:"make gear",
	               items: [{type:"hframe", items:["モジュール:", {id:"m", type:"number", value: 2.0}]},
	                       {type:"hframe", items:["歯数:", {id:"n", type:"number", value: 15}]}
	                      ]};
	let result = dialog.modalDialog(content, ["OK","Cancel"]);
	if (result && result.button == 0) { // button 0 = OK
		let dst = mqdocument.objects[mqdocument.currentObjectIndex];
		let r = involuteGear(dst, parseFloat(result.values["m"]), parseFloat(result.values["n"]));
		console.log("pitch R=" + r.r);
	}
} else {
	exports["involuteGear"] = involuteGear;
}
