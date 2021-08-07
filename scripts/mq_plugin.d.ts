
interface VecXYZ { x: number, y: number, z: number }
interface VecXYZW { x: number, y: number, z: number, w: number }

declare module "mqdocument" {
    export interface MQDocument {
        objects: ObjectList;
        materials: MaterialList;
        scene: MQScene;
        currentObjectIndex: number;
        currentObjectIndex: number;
        compact(): void;
        triangulate(v: VecXYZ[]): numvber[];
        clearSelect(flags: number = 0): void;
        isVertexSelected(obj: number, index: number): boolean;
        setVertexSelected(obj: number, index: number, select: boolean): void;
        isFaceSelected(obj: number, index: number): boolean;
        setFaceSelected(obj: number, index: number, select: boolean): void;
        getPluginData(key: string): string; // EXPERIMENTAL
        setPluginData(key: string, value: string): void; // EXPERIMENTAL
        setDrawProxyObject(obj: MQObject, proxy: MQObject); // EXPERIMENTAL
        getGlobalMatrix(obj: MQObject): number[]; // internal use.
        getObjectByName(name: string): MQObject | null; // .core.js
        addEventListener(event: string, lisntener: (e: any) => void); // .core.js
    }
    export class MQObject {
        constructor(name?: string);
        readonly id: number;
        readonly index: number;
        name: string;
        type: number;
        depth: number;
        compact(): void;
        clear(): void;
        freeze(flags: number = 0): void;
        merge(src: MQObject): void;
        clone(): MQObject;
        optimizeVertex(distance: number): void;
        verts: VertexList;
        faces: FaceList;
        selected: boolean;
        visible: boolean;
        locked: boolean;
        transform: MQTransform;
        wireframe: boolean; // EXPERIMENTAL
        globalMatrix: number[]; // EXPERIMENTAL
        applyTransform(matrix: { applyTo(p: VecXYZ): void }): void; // .core.js
        getChildren(recursive?: boolean): MQObject[]; // .core.js
    }
    export interface ObjectList extends Array<MQObject> {
        append(obj: MQObject): number;
        remove(objOrIndex: MQObject | number): void;
    }
    export interface MaterialList extends Array<MQMaterial> {
        append(obj: MQMaterial): number;
        remove(objOrIndex: MQMaterial | number): void;
    }
    export interface VertexList extends Array<MQVertex> {
        append(v: VecXYZ | number[] | number, y?: numver, z?: number): number;
        remove(index: number): void;
        push(v: VecXYZ): number;
    }
    export interface FaceList extends Array<MQFace> {
        append(points: number[], material: number): number;
        remove(index: number): void;
        push(face: { points: VecXYZ[], material: number }): number;
    }
    export interface MQVertex {
        readonly id: number;
        readonly refs: number;
        x: number;
        y: number;
        z: number;
    }
    export interface MQFace {
        readonly id?: number;
        readonly index?: number;
        readonly points?: nunber[];
        readonly uv?: { u: number, v: number }[];
        material: number;
        visible?: boolean;
        invert?(): void;
    }
    export class MQMaterial {
        constructor(name?: string);
        readonly id: number;
        readonly index: number;
        name: string;
        texture: string;
        color: MQColor;
        ambientColor: MQColor;
        emissionColor: MQColor;
        specularColor: MQColor;
        power: number;
        ambient: number;
        emission: number;
        specular: number;
        reflection: number;
        refraction: number;
        doubleSided: bool;
        selected: bool;
        shaderType: int;
    }
    export interface MQScene {
        cameraPosition: VecXYZ;
        cameraLookAt: VecXYZ;
        rotationCenter: VecXYZ;
        cameraAngle: MQangle;
        zoom: number; // camera zoom
        fov: number; // camera fov
    }
    export interface MQangle {
        pitch: number; // around x axis
        head: number; // around y axis
        bank: number; // around z axis
    }
    export interface MQColor {
        r: number;
        g: number;
        b: number;
        a?: number;
    }
    export interface MQTransform {
        position: VecXYZ;
        scale: VecXYZ;
        rotation: MQangle;
        matrix: number[];
    }
}

declare module "mqwidget" {
    type WidgetSpec = string | { type: string, items?: WidgetSpec[], [key: string]: any };
    export interface FormHandle {
        width: number;
        height: number;
        getValue(id: string): string;
        setValue(id: string, v: string | number): void;
        setContent(content: FormSpec): void;
        close(): void;
    }
    export type FormSpec = { title: string, items: WidgetSpec[] };
    export function createForm(content: FormSpec): FormHandle;
    export function modalDialog(content: FormSpec, buttons: string[]): number;
    export function fileDialog(): import("fs").File;
    export function folderDialog(): any;
    export function alertDialog(message: string, title: string = ""): void;
}

declare module "child_process" {
    export function exec(cmd: string): any;
    export function execSync(cmd: string): any;
}

declare module "fs" {
    export type File = any;
    export function open(path: string, write: boolean): File;
    export function readFile(path: string): any;
    export function writeFile(path: string, content: string): any;
}

declare module "bsptree" {
    // Experimental implementation of BSP tree.
    export type BSPPolygon = { vertices: VecXYZ[], plane: any, src?: BSPPolygon, [key: string]: any };
    export class BSPTree {
        constructor(polygons: any[]);
        build(polygons: any[], epsilon: number): void;
        raycast(ray: { origin: VecXYZ, direction: VecXYZ }, epsilon?: number): VecXYZ | null;
        crassifyPoint(point: VecXYZ, epsilon: number): number;
        clipPolygons(polygons: BSPPolygon[], inv: boolean, epsilon: number): BSPPolygon[];
        splitPolygons(src: BSPPolygon[], resultI: BSPPolygon[] | null, resultO: BSPPolygon[] | null, epsilon: number): void;
    }
}

// .core.js
declare module "geom" {
    export class Vector3 {
        x: number;
        y: number;
        z: number;
        constructor(x: number | VecXYZ | number[] = 0, y: number = 0, z: number = 0);
        puls(v: VecXYZ): Vector3;
        minus(v: VecXYZ): Vector3;
        mul(v: number): Vector3;
        div(v: number): Vector3;
        dot(v: VecXYZ): number;
        cross(v: VecXYZ): Vector3;
        length(): number;
        distanceTo(v: VecXYZ): number;
        distanceToSqr(v: VecXYZ): number;
        unit(): Vector3;
        lerp(v: VecXYZ, t: number): Vector3;
        clone(): Vector3;
        toString(): string;
    }
    export class Quaternion {
        x: number;
        y: number;
        z: number;
        w: number;
        constructor(x: number = 0, y: number = 0, z: number = 0, w: number = 1);
        dot(v: VecXYZW): number;
        applyTo(v: VecXYZ): Vector3;
        multiply(q: Quaternion): Quaternion;
        length(): number;
        clone(): Vector3;
        toString(): string;
        static fromAxisAngle(v: VecXYZ, rad: number): Quaternion;
        static fromAngle(x: number, y: number, z: number): Quaternion;
        static fromVectors(v: Vector3, v: Vector3): Quaternion;
    }
    export class Matrix4 {
        constructor(mat: number[]);
        m: number[16];
        applyTo(v: VecXYZ): Vector3;
        mul(m: Matrix4): Matrix4;
        static multiply(m1: Matrix4, m2: Matrix4, result: Matrix4): Matrix4;
        static scaleMatrix(x: number, y: number, z: number): Matrix4;
        static translateMatrix(x: number, y: number, z: number): Matrix4;
        static rotateMatrix(x: number, y: number, z: number, rad: number): Matrix4;
        static fromQuaternion(q: VecXYZW): Matrix4;
    }
    export class Plane {
        constructor(normal: Vector3, w: number);
        normal: Vector3;
        w: number;
        distanceTo(v: Vector3): number
        flipped(): Plane;
        toString(): string;
        static fromPoints(a: Vector3, b: Vector3, c: Vector3): Plane;
    }
}

// global
declare type MQDocument = import("mqdocument").MQDocument;
declare type MQObject = import("mqdocument").MQObject;
declare var MQObject: { new(name?: string): MQObject };
declare type MQMaterial = import("mqdocument").MQMaterial;
declare var MQMaterial: { new(name?: string): MQMaterial };
declare var mqdocument: import("mqdocument").MQDocument;
declare var process: { [key: string]: any };
