//@ts-check
export class PrimitiveModeler {
    constructor(object, material) {
        this.object = object || { verts: [] };
        this.material = material || 0;
        this.vertices = this.object.verts;
        this.transform = null;
    }
    addFace(points) {
        return this.object.faces.append(points, this.material);
    }
    addVertex(x, y, z) {
        if (this.transform) {
            return this.vertices.append(this.transform.applyTo({ x: x, y: y, z: z }));
        }
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
     * @param {{radius?:number}} spec
     */
    tetrahedron(spec = {}) {
        let r = spec.radius ?? 1;
        let ofs = this.vertices.length;
        let x = Math.sqrt(2 / 3) * r, y = - 1 / 3 * r, z = Math.sqrt(1 - 1 / 9) * r;
        [
            [0, r, 0],
            [0, y, z],
            [-x, y, -z / 2],
            [x, y, -z / 2],
        ].forEach(v => this.addVertex(v[0], v[1], v[2]));
        [
            [0, 1, 2], [0, 2, 3], [0, 3, 1], [3, 2, 1],
        ].forEach(f => this.addFace(f.map(p => p + ofs)));
    }
    /**
     * @param {{radius?:number}} spec
     */
    hexahedron(spec = {}) {
        let r = spec.radius ?? 1;
        this.cube({ size: r * 2 / Math.sqrt(3) });
    }
    /**
     * @param {{radius?:number}} spec
     */
    octahedron(spec = {}) {
        let r = spec.radius ?? 1;
        let ofs = this.vertices.length;
        [
            [0, r, 0],
            [r, 0, 0],
            [0, 0, r],
            [-r, 0, 0],
            [0, 0, -r],
            [0, -r, 0],
        ].forEach(v => this.addVertex(v[0], v[1], v[2]));
        [
            [0, 1, 2], [0, 2, 3], [0, 3, 4], [0, 4, 1],
            [5, 2, 1], [5, 3, 2], [5, 4, 3], [5, 1, 4],
        ].forEach(f => this.addFace(f.map(p => p + ofs)));
    }
    /**
     * @param {{radius?:number}} spec
     */
    dodecahedron(spec = {}) {
        let r = spec.radius ?? 1;
        let ofs = this.vertices.length;
        let b = (1 + Math.sqrt(5)) / 2, a = 1 / b;
        [
            [-1, -1, -1], [-1, -1, 1], [-1, 1, -1], [-1, 1, 1],
            [1, -1, -1], [1, -1, 1], [1, 1, -1], [1, 1, 1],
            [0, -a, -b], [0, -a, b], [0, a, -b], [0, a, b],
            [-a, -b, 0], [-a, b, 0], [a, -b, 0], [a, b, 0],
            [-b, 0, -a], [b, 0, -a], [-b, 0, a], [b, 0, a],
        ].forEach(v => this.addVertex(v[0] * r, v[1] * r, v[2] * r));
        [
            [13, 15, 7, 11, 3],
            [15, 6, 17, 19, 7],
            [6, 10, 8, 4, 17],
            [10, 2, 16, 0, 8],
            [16, 18, 1, 12, 0],
            [15, 13, 2, 10, 6],
            [13, 3, 18, 16, 2],
            [3, 11, 9, 1, 18],
            [8, 0, 12, 14, 4],
            [7, 19, 5, 9, 11],
            [17, 4, 14, 5, 19],
            [9, 5, 14, 12, 1]
        ].forEach(f => this.addFace(f.map(p => p + ofs)));
    }
    /**
     * @param {{radius?:number}} spec
     */
    icosahedron(spec = {}) {
        let r = spec.radius ?? 1;
        let ofs = this.vertices.length;
        let x = (Math.sqrt(5) + 1) / 2;
        [
            [-1, x, 0], [1, x, 0], [-1, -x, 0], [1, -x, 0],
            [0, -1, x], [0, 1, x], [0, -1, -x], [0, 1, -x],
            [x, 0, -1], [x, 0, 1], [-x, 0, -1], [-x, 0, 1],
        ].forEach(v => this.addVertex(v[0] * r, v[1] * r, v[2] * r));
        [
            [0, 1, 5], [0, 7, 1], [0, 10, 7], [0, 11, 10], [0, 5, 11],
            [1, 9, 5], [5, 4, 11], [11, 2, 10], [10, 6, 7], [7, 8, 1],
            [3, 4, 9], [3, 2, 4], [3, 6, 2], [3, 8, 6], [3, 9, 8],
            [4, 5, 9], [2, 11, 4], [6, 10, 2], [8, 7, 6], [9, 1, 8]
        ].forEach(f => this.addFace(f.map(p => p + ofs)));
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
        ].forEach(v => this.addVertex(v[0], v[1], v[2]));
        this.addFace([st, st + 1, st + 2, st + 3]);
    }
}
