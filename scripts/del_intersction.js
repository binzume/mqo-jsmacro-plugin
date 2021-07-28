"use strict";
// delete intersected faces.

const epsilon = 0.00001;

// return determinant
function det3(v1, v2, v3) {
	return v1.x * v2.y * v3.z + v1.y * v2.z * v3.x + v1.z * v2.x * v3.y
		 - v1.x * v2.z * v3.y - v1.y * v2.x * v3.z - v1.z * v2.y * v3.x;
}

// return distance
function rayTriangleDist(origin, ray, p0, p1, p2) {
	var v1 = {x: p1.x-p0.x , y: p1.y-p0.y, z: p1.z-p0.z };
	var v2 = {x: p2.x-p0.x , y: p2.y-p0.y, z: p2.z-p0.z };
	var det = det3(v1, v2, ray) * -1;
	if (det < epsilon && det > -epsilon ) return null;
	var vr = {x: origin.x-p0.x , y: origin.y-p0.y, z: origin.z-p0.z };
	var u = -det3(vr, v2, ray) / det;;
	if (u >= 0) {
		var v = -det3(v1, vr, ray) / det;
		if (v >= 0 && u + v <= 1) {
			return det3(v1, v2, vr) / det;
		}
	}
    return null;
}

// return intersection of line(lp0, lp1) and triangle(p0, p1, p2)
function rayTrianglePos(lp0, lp1, p0, p1, p2) {
	var ray = {x: lp1.x-lp0.x , y: lp1.y-lp0.y, z: lp1.z-lp0.z };
	var d = rayTriangleDist(lp0, ray, p0, p1, p2);
	if (d != null && d > epsilon && d < 1 - epsilon) {
		// lp0 + ray * t;
		return {x: lp0.x + ray.x * d, y: lp0.y + ray.y * d, z: lp0.z + ray.z * d};
	}
	return null;
}

function bBox(points) {
	var max = {x: points[0].x, y: points[0].y, z: points[0].z};
	var min = {x: points[0].x, y: points[0].y, z: points[0].z};
	points.forEach((p,idx) => {
		if (max.x < p.x) max.x = p.x;
		if (max.y < p.y) max.y = p.y;
		if (max.z < p.z) max.z = p.z;
		if (min.x > p.x) min.x = p.x;
		if (min.y > p.y) min.y = p.y;
		if (min.z > p.z) min.z = p.z;
	});
	return [min, max];
}

function aabb(b1, b2) {
	var min1 = b1[0], max1 = b1[1];
	var min2 = b2[0], max2 = b2[1];
	return (min1.x <= max2.x && max1.x >= min2.x
		&& min1.y <= max2.y && max1.y >= min2.y
		&& min1.z <= max2.z && max1.z >= min2.z)
}

document.compact();

document.objects.forEach((obj, objindex) => {
	if (!obj.selected) return;
	var bbox = obj.faces.map((f,idx) => {
		return {box: bBox(f.points.map( (i) => { return obj.verts[i] } )), face: f};
	});

	bbox.forEach((box1,idx) => {
		if (!document.isFaceSelected(objindex, idx)) return;
		var points = box1.face.points.map( (i) => { return obj.verts[i] } );
		var found = false;
		bbox.forEach((box2,idx2) => {
			if (found) return;
			if (idx == idx2 || !aabb(box1.box, box2.box)) return;
			if (box2.face.points.length < 3) return;
			var points2 = box2.face.points.map( (i) => { return obj.verts[i] } );
			for (var i=0; i < points.length; i++) {
				if (points2.length == 3) {
					if (rayTrianglePos(points[i], points[(i+1)%points.length], points2[0], points2[1], points2[2])) {
						found = true;
						break;
					}
				} else {
					var indices = document.triangulate(points2);
					for (var j = 0; j < indices.length / 3; j++) {
						if (rayTrianglePos(points[i], points[(i+1)%points.length], points2[indices[j*3]], points2[indices[j*3+1]], points2[indices[j*3+2]])) {
							found = true;
							break;
						}
					}
				}
			}
		});
		if (found) {
		 	delete obj.faces[idx];
		}
	});
});

