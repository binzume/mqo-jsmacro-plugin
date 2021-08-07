/// @ts-check
import { Vector3, Matrix4, Quaternion } from "geom"
import { CSGObject, CSGPrimitive, csgToObject } from "./modules/csg.js"
import { PrimitiveModeler } from "./modules/primitives.js"

const yVec = new Vector3(0, 1, 0);
class SimpleTube {
    constructor(dstObj) {
        this.tubeSpec = { type: 'cylinder', radius: 2 };
        this.jointSpec = { type: 'sphere', radius: 4 };
        this.m = new PrimitiveModeler(dstObj);
    }
    mkJoint(v, refs) {
        if (refs >= 2) {
            this.m.transform = Matrix4.translateMatrix(v.x, v.y, v.z);
            this.m[this.jointSpec.type](this.jointSpec);
        }
        return 0;
    }
    mkTube(v1, v2, j1, j2) {
        let c = v1.plus(v2).mul(0.5);
        let q = Quaternion.fromVectors(yVec, v2.minus(v1));
        let tr = Matrix4.translateMatrix(c.x, c.y, c.z)
            .mul(Matrix4.fromQuaternion(q))
            .mul(Matrix4.scaleMatrix(1, v1.distanceTo(v2), 1));
        this.m.transform = tr;
        this.m[this.tubeSpec.type](this.tubeSpec);
    }
    flush() { }
}

class CSGTube {
    constructor(dstObj) {
        /** @type {MQObject} */
        this.dst = dstObj;
        /** @type {CSGObject[]} */
        this.objects = [];
        this.tubeSpec = { type: 'cylinder', radius: 2 };
        this.jointSpec = { type: 'sphere', radius: 4 };
    }
    mkJoint(v, refs) {
        return this._addObj(CSGPrimitive.sphere(this.jointSpec).transformed(Matrix4.translateMatrix(v.x, v.y, v.z)));
    }
    mkTube(v1, v2, j1, j2) {
        let c = v1.plus(v2).mul(0.5);
        let q = Quaternion.fromVectors(yVec, v2.minus(v1));
        let tr = Matrix4.translateMatrix(c.x, c.y, c.z)
            .mul(Matrix4.fromQuaternion(q))
            .mul(Matrix4.scaleMatrix(1, v1.distanceTo(v2), 1));
        this._addObj(CSGPrimitive.cylinder(this.tubeSpec).transformed(tr));
    }
    _addObj(csg) {
        let id = this.objects.length;
        csg.polygons.forEach((p, i) => p.shared = [{ id: i, material: 0 }, { id: id }]);
        this.objects.push(csg);
        return id;
    }
    flush() {
        if (this.objects.length == 0) {
            return;
        }
        // TODO: clipPolygons instead of union to improve performance.
        let o = this.objects.reduce((acc, o) => acc.union(o));
        csgToObject(this.dst, o, true, true);
    }
}


function wire2tube(srcObj, tubeWriter) {
    let jointmap = new Map();
    for (let i = 0; i < srcObj.verts.length; i++) {
        let v = srcObj.verts[i];
        jointmap.set(i, tubeWriter.mkJoint(v, v.refs));
    }

    let vv = new Set();
    for (let f of srcObj.faces) {
        let points = f.points;
        for (let i = 0; i < points.length; i++) {
            let p1 = points[i];
            let p2 = points[(i + 1) % points.length];
            let k = p1 * 10000 + p2;
            if (vv.has(k)) {
                continue;
            }
            vv.add(k);
            vv.add(p2 + p1 * 10000);

            let v1 = new Vector3(srcObj.verts[p1]);
            let v2 = new Vector3(srcObj.verts[p2]);
            tubeWriter.mkTube(v1, v2, jointmap.get(p1), jointmap.get(p2));
        }
    }
    tubeWriter.flush();
}

export { SimpleTube, CSGTube, wire2tube }

// Example
let srcObj = mqdocument.objects[mqdocument.currentObjectIndex];
let dstObj = new MQObject();
wire2tube(srcObj, new SimpleTube(dstObj));
mqdocument.objects.append(dstObj);
