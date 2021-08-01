import { Vector3, Quaternion } from "geom"
import { BSPTree } from "bsptree"
import { toCSGPolygons, csgToObject } from "./modules/csg.js"

let polygons = toCSGPolygons(mqdocument.objects[0]);
let bsp = new BSPTree(polygons);
let obj = mqdocument.objects[mqdocument.currentObjectIndex];

let origin = new Vector3(mqdocument.scene.cameraPosition);
let dir = new Vector3(mqdocument.scene.cameraLookAt).minus(origin).unit();

function cast(ray) {
    let intersect = bsp.raycast(ray);
    if (intersect) {
        // console.log(intersect);
        let oi = obj.verts.append(origin);
        obj.faces.append([oi, obj.verts.append(intersect)], 0);
    }
}

let startTime = Date.now();

let width = 1000, height = 100, fov = mqdocument.scene.fov;
let ray = { origin: origin, direction: dir };
for (let y = 0; y < height; y++) {
    let av = y / height - 0.5;
    for (let x = 0; x < width; x++) {
        let ah = x / width - 0.5;
        let q = Quaternion.fromAngle(av * fov, ah * fov, 0);
        ray.direction = q.applyTo(dir);
        cast(ray);
    }
}

console.log("" + (Date.now() - startTime) + "ms");
