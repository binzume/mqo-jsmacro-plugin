// @ts-check
/// <reference path="../mq_plugin.d.ts" />
import { BSPTree } from "bsptree"
import { Vector3, Plane } from "geom"
import { PrimitiveModeler } from "./primitives.js"

class Polygon {
	/**
	 * @param {Vector3[]} vertices 
	 * @param {any} shared
	 * @param {Plane} plane
	 */
	constructor(vertices, shared = null, plane = null) {
		/** @type {Vector3[]} */
		this.vertices = vertices;
		this.shared = shared;
		/** @type {Plane} */
		this.plane = plane || Plane.fromPoints(vertices[0], vertices[1], vertices[2]);
	}
	clone() {
		return new Polygon(this.vertices.slice(), this.shared, this.plane);
	}
	flip() {
		this.vertices.reverse();
		this.plane = this.plane.flipped();
	}
}

class CSGObject {
	static EPSILON = 1e-6;
	/**
	 * @param {Polygon[]} polygons 
	 */
	constructor(polygons) {
		/** @type {Polygon[]} */
		this.polygons = polygons || [];
	}
	clone() {
		return new CSGObject(this.polygons.map(function (p) { return p.clone(); }));
	}
	/**
	 * union csg objects.
	 * Inspired by csg.js https://evanw.github.io/csg.js/
	 * @param {CSGObject} csg 
	 */
	union(csg) {
		let a = new BSPTree(this.polygons);
		let b = new BSPTree(csg.polygons);
		let ap = this.bsp_clipPolygons(b, this.polygons);
		let bp = this.bsp_clipPolygons(a, csg.polygons);
		bp.forEach(function (p) { p.flip() });
		bp = this.bsp_clipPolygons(a, bp);
		bp.forEach(function (p) { p.flip() });
		return new CSGObject(ap.concat(bp));
	}
	/**
	 * @param {CSGObject} csg 
	 */
	subtract(csg) {
		let a = new BSPTree(this.polygons);
		let b = new BSPTree(csg.polygons);
		let ap = this.clone().polygons, bp = csg.polygons;
		ap.forEach(function (p) { p.flip() });
		ap = this.bsp_clipPolygons(b, ap);
		bp = this.bsp_clipPolygons(a, bp, true);
		ap.forEach(function (p) { p.flip() });
		bp.forEach(function (p) { p.flip() });
		bp = this.bsp_clipPolygons(a, bp, true);
		return new CSGObject(ap.concat(bp));
	}
	/**
	 * @param {CSGObject} csg 
	 */
	intersect(csg) {
		let a = new BSPTree(this.polygons);
		let b = new BSPTree(csg.polygons);
		let ap = this.clone().polygons, bp = csg.polygons;
		ap.forEach(function (p) { p.flip() });
		bp = this.bsp_clipPolygons(a, bp, true);
		bp.forEach(function (p) { p.flip() });
		ap = this.bsp_clipPolygons(b, ap, true);
		bp = this.bsp_clipPolygons(a, bp, true);
		ap = ap.concat(bp);
		ap.forEach(function (p) { p.flip() });
		return new CSGObject(ap);
	}
	inverse() {
		let csg = this.clone();
		csg.polygons.map(function (p) { p.flip(); });
		return csg;
	}
	transformed(tr) {
		return new CSGObject(this.polygons.map((p) => {
			return new Polygon(p.vertices.map((v) => {
				return tr.applyTo(v);
			}), p.shared);
		}));
	}

	bsp_clipPolygons(bsp, polygons, inv = false) {
		return bsp.clipPolygons(polygons, inv, CSGObject.EPSILON).map(p => p.src ? new Polygon(p.vertices.map(v => new Vector3(v)), p.src.shared, p.src.plane) : p.clone())
	}
}

class CSGPrimitive {
	static newGenerator() {
		/**@type {PrimitiveModeler & {polygons: Polygon[]}} */
		// @ts-ignore
		let m = new PrimitiveModeler();
		m.polygons = [];
		m.addVertex = function addVertex(x, y, z) {
			this.vertices.push(new Vector3(x, y, z));
		};
		m.addFace = function addFace(points) {
			this.polygons.push(new Polygon(points.reverse().map(p => this.vertices[p])));
		};
		return m;
	}
	static cube(spec) {
		let g = this.newGenerator();
		g.cube(spec);
		return new CSGObject(g.polygons);
	}
	static box(spec) {
		let g = this.newGenerator();
		g.box(spec);
		return new CSGObject(g.polygons);
	}
	static cylinder(spec, s) {
		let g = this.newGenerator();
		g.cylinder(spec, s);
		return new CSGObject(g.polygons);
	}
	static sphere(spec, sh, sv) {
		let g = this.newGenerator();
		g.sphere(spec, sh, sv);
		return new CSGObject(g.polygons);
	}
	static plane(spec) {
		let g = this.newGenerator();
		g.plane(spec);
		return new CSGObject(g.polygons);
	}
}

let epsilon = 1e-10; // TODO: use CSGObject.EPSILON

function collinear(v0, v1, v2) {
	let v01 = v1.minus(v0);
	let v02 = v2.minus(v0);
	let d = v01.length() * v02.length() - Math.abs(v01.dot(v02));
	return d < epsilon;
}

function overwrap(a1, a2, b1, b2) {
	let a1a2 = a2.minus(a1);
	let b1b2 = b2.minus(b1);
	if (a1a2.dot(b1b2) > a1a2.length() * b1b2.length() - epsilon) {
		let a1b1 = b1.minus(a1);
		if (a1b1.length() < a1a2.length() - epsilon) {
			if (a1a2.dot(a1b1) > a1a2.length() * a1b1.length() - epsilon) {
				return true;
			}
		}
		let b1a1 = a1.minus(b1);
		if (b1a1.length() < b1b2.length() - epsilon) {
			if (b1b2.dot(b1a1) > b1b2.length() * b1a1.length() - epsilon) {
				return true;
			}
		}
	}
	return false;
}


/**
 * @param {Vector3} p 
 * @param {Vector3} a 
 * @param {Vector3} b 
 * @param {Vector3} c 
 */
function insideTriangle(p, a, b, c) {
	const ab = b.minus(a), bc = c.minus(b), ca = a.minus(c);
	const c1 = ab.cross(p.minus(a)), c2 = bc.cross(p.minus(b)), c3 = ca.cross(p.minus(c));
	return c1.dot(c2) > 0 && c2.dot(c3) > 0 && c3.dot(c1) > 0;
}

/**
 * @param {Vector3[]} vv 
 */
function cleanEdge(vv) {
	const threshold = CSGObject.EPSILON * CSGObject.EPSILON * 100;
	do {
		var update = false;
		for (let k = 0; k < vv.length; k++) {
			let d = (k + 1) % vv.length;
			if (collinear(vv[k], vv[d], vv[(k + 2) % vv.length])) {
				vv.splice(d, 1);
				update = true;
			}
		}
	} while (update);
	do {
		var update = false;
		if (vv.length > 5) {
			for (let i = 0; i < vv.length; i++) {
				let iv0 = vv[(i + vv.length - 1) % vv.length];
				let iv1 = vv[i];
				let iv2 = vv[(i + 1) % vv.length];
				for (let j = 0; j < vv.length; j++) {
					if (i == j) {
						continue;
					}
					let jv0 = vv[(j + 1) % vv.length];
					let jv1 = vv[j];
					let jv2 = vv[(j + vv.length - 1) % vv.length];
					if (iv0.distanceToSqr(jv0) < threshold && iv1.distanceToSqr(jv1) < threshold && iv2.distanceToSqr(jv2) < threshold) {
						let ok = true;
						for (let k = i + 2; k < i + vv.length - 1; k++) {
							if (insideTriangle(vv[k % vv.length], iv0, iv1, iv2)) {
								ok = false;
								break;
							}
						}
						if (!ok) {
							continue;
						}
						if (i < j) {
							vv.splice(j, 1);
							vv.splice(i, 1);
						} else {
							vv.splice(j, 1);
							vv.splice(i, 1);
						}
						update = true;
						i = vv.length;
						break;
					}
				}
			}
		}
	} while (update);
	return vv;
}

function tryMerge(p1, p2) {
	let vs1 = p1.vertices
	let vs2 = p2.vertices
	for (var i = 0; i < vs1.length; i++) {
		let i2 = (i + 1) % vs1.length;
		for (var j = 0; j < vs2.length; j++) {
			let j2 = (j - 1 + vs2.length) % vs2.length;
			if (overwrap(vs1[i], vs1[i2], vs2[j], vs2[j2])) {
				let vv = (i2 > i) ? vs1.slice(i2).concat(vs1.slice(0, i + 1)) : vs1.slice(i2, i + 1);
				vv = (j > j2) ? vv.concat(vs2.slice(j)).concat(vs2.slice(0, j2 + 1)) : vv.concat(vs2.slice(j, j2 + 1));
				vv = cleanEdge(vv);
				if (vv.length < 3) {
					return null;
				}
				return new Polygon(vv, p1.shared);
			}
		}
	}
	return null;
}

/**
 * @param {Polygon[]} polygons 
 */
function mergePolygons(polygons) {
	/** @type {Polygon[]} */
	var mpolygons = [];
	var byface = new Map();
	polygons.forEach((p) => {
		if (!p.shared) {
			mpolygons.push(p);
			return;
		}
		var k = p.shared[1].id + "," + p.shared[0].id;
		var a = byface.get(k) || [];
		a.push(p);
		byface.set(k, a);
	});

	let merged = true;
	while (merged) {
		merged = false;
		byface.forEach((fs) => {
			for (var i = 0; i < fs.length; i++) {
				if (!fs[i]) continue;
				for (var j = i + 1; j < fs.length; j++) {
					if (!fs[j]) continue;
					if (fs[i].vertices.length < 3) continue;
					var m = tryMerge(fs[i], fs[j]);
					if (m) {
						delete fs[j];
						fs[i] = m;
						merged = true;
					}
				}
			}
		});
	}

	byface.forEach((fs) => {
		mpolygons = mpolygons.concat(fs);
	});
	console.log("merged" + polygons.length + " -> " + mpolygons.length);
	return mpolygons;
}

function getVertIndex(pos, obj, verts) {
	const threshold = CSGObject.EPSILON * CSGObject.EPSILON * 100;
	// !!! O(verts.length)
	let v = verts.find(function (e) {
		return e[0].distanceToSqr(pos) < threshold;
	});
	if (v !== undefined) {
		return v[1];
	}
	let i = obj.verts.append(pos);
	verts.push([pos, i]);
	return i;
}

/**
 * @param {MQObject} dst 
 * @param {CSGObject} csg 
 * @param {boolean} mergeFaces 
 * @param {boolean} mergeVerts 
 */
function csgToObject(dst, csg, mergeFaces, mergeVerts) {
	let polygons = csg.clone().polygons;
	if (mergeFaces) {
		polygons = mergePolygons(polygons);
	}
	if (mergeVerts) {
		let vv = [];
		polygons.forEach((p) => {
			let a = p.vertices.map((v) => getVertIndex(v, dst, vv));
			dst.faces.append(a.reverse(), p.shared ? p.shared[0].material : null);
		});
	} else {
		polygons.forEach((p) => {
			let a = p.vertices.map((v) => dst.verts.append(v));
			dst.faces.append(a.reverse(), p.shared ? p.shared[0].material : null);
		});
	}
}

/**
 * @param {MQObject} obj 
 * @returns {Polygon[]}
 */
function toCSGPolygons(obj) {
	return obj.faces.reduce((acc, f) => {
		if (!f || f.points.length < 3) return acc;
		let points = f.points.map((i) => { return obj.verts[i] });
		if (points.length == 3) {
			let vs = points.reverse();
			let p = new Polygon(vs, [f, obj]);
			if (p.plane.w == p.plane.w) {
				acc.push(p);
			}
		} else {
			let trpoints = mqdocument.triangulate(points).map((i) => { return points[i] });
			for (let i = 0; i < trpoints.length / 3; i++) {
				let vs = trpoints.slice(i * 3, i * 3 + 3).reverse();
				let p = new Polygon(vs, [f, obj]);
				if (p.plane.w == p.plane.w) {
					acc.push(p);
				}
			}
		}
		return acc;
	}, []);
}

export { CSGObject, CSGPrimitive, csgToObject, toCSGPolygons };
