

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
    box(w, h, d) {
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
        ].forEach(f => this.addFace(f)); // TODO offset
    }
    cube(size = 1) {
        this.box(size, size, size);
    }
    sphere(r, sh, sv) {
        for (let i = 0; i <= sv; i++) {
            let t = i / sv * Math.PI;
            let y = Math.cos(t) * r;
            let r2 = Math.sin(t) * r;
            for (let j = 0; j < sh; j++) {
                let t2 = j / sh * 2 * Math.PI;
                this.addVertex(Math.cos(t2) * r2, y, Math.sin(t2) * r2);
            }
        }
        for (let i = 0; i < sv; i++) {
            let i1 = i * sh;
            let i2 = (i + 1) * sh;
            for (let j = 0; j < sh; j++) {
                let j2 = (j + 1) % sh;
                if (i == 0) {
                    this.addFace([i1 + j, i2 + j, i2 + j2]);
                    continue;
                } else if (i == sv - 1) {
                    this.addFace([i1 + j, i2 + j, i1 + j2]);
                    continue;
                }
                this.addFace([i1 + j, i2 + j, i2 + j2, i1 + j2]);
            }
        }
    }
    cylinder(r, h, s = 20) {
        let st = this.vertices.length;
        let top = [], bottom = [];
        for (let i = 0; i < s; i++) {
            let t = i / s * 2 * Math.PI;
            this.addVertex(Math.cos(t) * r, h / 2, Math.sin(t) * r);
            this.addVertex(Math.cos(t) * r, -h / 2, Math.sin(t) * r);
        }
        for (let i = 0; i < s; i++) {
            this.addFace([st + i * 2, st + i * 2 + 1, st + (i * 2 + 3) % (s * 2), st + (i * 2 + 2) % (s * 2)]);
            top.push(st + i * 2);
            bottom.push(st + i * 2 + 1);
        }
        this.addFace(top);
        this.addFace(bottom.reverse());
    }
    plane(size = 1) {
        let st = this.vertices.length;
        [
            [-size / 2, 0, -size / 2],
            [size / 2, 0, -size / 2],
            [size / 2, 0, size / 2],
            [-size / 2, 0, size / 2],
        ].forEach(v => this.addVertex(v));
        this.addFace([st, st + 1, st + 2, st + 3]);
    }
}

