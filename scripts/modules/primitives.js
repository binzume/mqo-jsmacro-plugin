//@ts-check
export class PrimitiveModeler {
    constructor(object, material) {
        this.object = object || { verts: [] };
        this.material = material || 0;
        this.vertices = this.object.verts;
    }
    addFace(points) {
        return this.object.faces.append(points, this.material);
    }
    addVertex(x, y, z) {
        return this.vertices.append(x, y, z);
    }
    /**
     * @param {{width?:number, height?:number, depth?:number}} spec 
     */
    box(spec = {}) {
        let w = spec.width ?? 1, h = spec.height ?? 1, d = spec.depth ?? 1;
        let ofs = this.vertices.length;
        [
            [-w / 2, -h / 2, -d / 2],
            [w / 2, -h / 2, -d / 2],
            [w / 2, h / 2, -d / 2],
            [-w / 2, h / 2, -d / 2],
            [-w / 2, -h / 2, d / 2],
            [w / 2, -h / 2, d / 2],
            [w / 2, h / 2, d / 2],
            [-w / 2, h / 2, d / 2],
        ].forEach(v => this.addVertex(v[0], v[1], v[2]));
        [
            [0, 1, 2, 3], [7, 6, 5, 4],
            [1, 0, 4, 5], [3, 2, 6, 7],
            [2, 1, 5, 6], [0, 3, 7, 4],
        ].forEach(f => this.addFace(f.map(p => p + ofs)));
    }
    /**
     * @param {{size?:number}} spec 
     */
    cube(spec = {}) {
        this.box({ width: spec.size, height: spec.size, depth: spec.size });
    }
    /**
     * @param {{radius?:number}} spec 
     */
    sphere(spec = {}, sh = 16, sv = 8) {
        let r = spec.radius ?? 1;
        let st = this.vertices.length;
        this.addVertex(0, r, 0);
        this.addVertex(0, -r, 0);
        for (let i = 1; i < sv; i++) {
            let t = i / sv * Math.PI;
            let y = Math.cos(t) * r;
            let r2 = Math.sin(t) * r;
            for (let j = 0; j < sh; j++) {
                let t2 = j / sh * 2 * Math.PI;
                this.addVertex(Math.cos(t2) * r2, y, Math.sin(t2) * r2);
            }
        }
        let ofs = st + 2;
        for (let i = 0; i < sv; i++) {
            let i1 = (i - 1) * sh;
            let i2 = (i) * sh;
            for (let j = 0; j < sh; j++) {
                let j2 = (j + 1) % sh;
                if (i == 0) {
                    this.addFace([st, i2 + j + ofs, i2 + j2 + ofs]);
                    continue;
                } else if (i == sv - 1) {
                    this.addFace([i1 + j + ofs, st + 1, i1 + j2 + ofs]);
                    continue;
                }
                this.addFace([i1 + j + ofs, i2 + j + ofs, i2 + j2 + ofs, i1 + j2 + ofs]);
            }
        }
    }
    /**
     * @param {{radius?:number, height?:number}} spec 
     */
    cylinder(spec = {}, s = 20) {
        let r = spec.radius ?? 1;
        let height = spec.height ?? 1;
        let st = this.vertices.length;
        let top = [], bottom = [];
        for (let i = 0; i < s; i++) {
            let t = i / s * 2 * Math.PI;
            this.addVertex(Math.cos(t) * r, height / 2, Math.sin(t) * r);
            this.addVertex(Math.cos(t) * r, -height / 2, Math.sin(t) * r);
        }
        for (let i = 0; i < s; i++) {
            this.addFace([st + i * 2, st + i * 2 + 1, st + (i * 2 + 3) % (s * 2), st + (i * 2 + 2) % (s * 2)]);
            top.push(st + i * 2);
            bottom.push(st + i * 2 + 1);
        }
        this.addFace(top);
        this.addFace(bottom.reverse());
    }
    /**
     * @param {{width?:number, height?:number}} spec 
     */
    plane(spec = {}) {
        let w = spec.width ?? 1, h = spec.height;
        let st = this.vertices.length;
        [
            [-w / 2, 0, -h / 2],
            [w / 2, 0, -h / 2],
            [w / 2, 0, h / 2],
            [-w / 2, 0, h / 2],
        ].forEach(v => this.addVertex(v));
        this.addFace([st, st + 1, st + 2, st + 3]);
    }
}
