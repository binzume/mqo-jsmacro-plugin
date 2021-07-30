// @ts-check
/// <reference path="mq_plugin.d.ts" />
import { createForm } from "mqwidget"
import { Vector3, Matrix4 } from "geom"
import { CSGObject, CSGPrimitive, toCSGPolygons, csgToObject } from "./modules/csg.js"
import { PrimitiveModeler } from "./modules/primitives.js"

// Node types
const TYPE_DEFAULT = 'DEFAULT';
const TYPE_PRIMITIVE = 'PRIMITIVE';
const TYPE_CSG = 'CSG';
const TYPE_OBJREF = 'OBJREF';
const TYPE_EVAL = 'EVAL'; // Deprecated
const TYPE_IGNORE = 'DISABLE';
const NODE_TYPES = [TYPE_DEFAULT, TYPE_PRIMITIVE, TYPE_CSG, TYPE_OBJREF, TYPE_IGNORE];

// Primitive types
const TYPE_CUBE = 'CUBE';
const TYPE_BOX = 'BOX';
const TYPE_SPHERE = 'SPHERE';
const TYPE_PLANE = 'PLANE';
const TYPE_CYLINDER = 'CYLINDER';
const PRIMITIVE_TYPES = [TYPE_CUBE, TYPE_BOX, TYPE_SPHERE, TYPE_CYLINDER, TYPE_PLANE];

// CSG types
const TYPE_UNION = 'UNION';
const TYPE_SUB = 'SUB';
const TYPE_AND = 'AND';
const TYPE_INV = 'INV';
const CSG_TYPES = [TYPE_UNION, TYPE_SUB, TYPE_AND, TYPE_INV];

const SETTING_KEY_PREFIX = 'CSG_OBJCT_';

class MeshBuilder {
    constructor() {
        this.slices = 32;
        /**@type {Record<string, CSGObject>} */
        this.csgObj = {};
        this.buildStartTime = 0;
        this.primitives = {
            CUBE: (p) => CSGPrimitive.cube(p.size),
            BOX: (p) => CSGPrimitive.box(p.width, p.height, p.depth),
            SPHERE: (p) => CSGPrimitive.sphere(p.radius, this.slices, this.slices / 2),
            CYLINDER: (p) => CSGPrimitive.cylinder(p.radius, p.height, this.slices),
        };
        this.evaluator = new CSGEval();
    }
    /**
     * @param {MQObject} obj 
     */
    build(obj) {
        obj.clear();
        obj.compact();
        this.buildStartTime = Date.now();
        this.buildCsg(obj);
        this.log("buildCsg");
        csgToObject(obj, this.csgObj[obj.name], true, true);
        this.log("finished.");
    }
    /**
     * @param {string} msg 
     */
    log(msg) {
        console.log("" + (Date.now() - this.buildStartTime) + "ms: " + msg);
    }
    /**
     * @param {MQObject} obj 
     */
    buildCsg(obj) {
        let d = mqdocument.getPluginData(SETTING_KEY_PREFIX + obj.id);
        let spec = (d && d != "") ? JSON.parse(d) : {};
        this.log("building... " + spec.type + " " + obj.name);
        if (spec.type == TYPE_PRIMITIVE) {
            this.csgObj[obj.name] = this.buildPrimitive(spec.primitive, obj);
        } else if (spec.type == TYPE_OBJREF) {
            this.buildCsg(mqdocument.getObjectByName(spec.objref.name));
            this.csgObj[obj.name] = this.csgObj[spec.objref.name];
        } else if (spec.type == TYPE_CSG) {
            this.csgObj[obj.name] = this.doCSG(spec.csg, obj);
        } else if (spec.type == TYPE_EVAL) {
            this.csgObj[obj.name] = this.evaluator.evalExp(spec.eval);
        } else {
            this.csgObj[obj.name] = new CSGObject(toCSGPolygons(obj));
        }
        this.log("built " + obj.name);
    }
    buildPrimitive(spec, obj) {
        let f = this.primitives[spec.type];
        if (!f) {
            throw "Not supported:" + spec.type;
        }
        let csg = f(spec);
        // Set material
        csg.polygons.forEach((p, i) => p.shared = [{ id: i, material: spec.material || 0 }, obj]);
        return csg;
    }
    getTransformed(obj) {
        return this.csgObj[obj.name].transformed(new Matrix4(obj.transform.matrix));
    }
    doCSG(c, obj) {
        let children = obj.getChildren();
        if (children.length == 0) {
            return new CSGObject([]);
        }
        for (let o of children) {
            this.buildCsg(o);
        }
        let csg = this.getTransformed(children.shift());
        if (c.type == TYPE_UNION || c.type == TYPE_INV) {
            for (let o of children) {
                csg = csg.union(this.getTransformed(o));
            }
        } else if (c.type == TYPE_SUB) {
            for (let o of children) {
                csg = csg.subtract(this.getTransformed(o));
            }
        } else if (c.type == TYPE_AND) {
            for (let o of children) {
                csg = csg.intersect(this.getTransformed(o));
            }
        }
        if (c.type == TYPE_INV) {
            csg = csg.inverse();
        }
        return csg;
    }

}

/**
 * evaluate CSG expression used in old version.
 * example: "base2=csg:base1&(box(90,160,90).mv(0,80,30))"
 * @deprecated
 */
class CSGEval {
    constructor(csgObjs) {
        /** @type {Record<string, (params:string[])=>CSGObject>} */
        this.factories = {};
        /** @type {Record<string, (stc:CSGObject, params:string[])=>CSGObject>} */
        this.modifiers = {};
        this.csgObjects = csgObjs;
    }
    /**
     * @deprecated
     * @param {string} exp
     */
    evalExp(exp) {
        let prefix = "_tmp", seq = 0, needclean = false;
        let csgs = this.csgObjects;
        let functions = this.factories;
        let methods = this.modifiers;
        exp = '(' + exp.replace(/\s+/, '') + ')';
        do {
            console.log(exp);
            var lastexp = exp;
            exp = exp.replace(/\(([0-9]+)\)/, "($1,)"); // (123) => (123,)
            exp = exp.replace(/\((\w+)\)/, "$1"); // (a) => a
            exp = exp.replace(/(\w+)\.(\w+)\(([^)]*)\)/g, (a, op1, f, params) => {
                if (!methods[f]) {
                    alert("ERROR func: " + op1 + ":" + f + " " + params);
                    throw ("ERROR func: " + op1 + ":" + f + " " + params);
                }
                let src = csgs[op1] ? csgs[op1].clone() : new CSGObject(toCSGPolygons(mqdocument.getObjectByName(op1)));
                csgs[prefix + seq] = methods[f](src, params.split(","));
                return prefix + (seq++);
            });
            exp = exp.replace(/\.?(\w+)\(([^)]*)\)/g, (a, f, params) => {
                if (a.startsWith(".")) return a;
                if (!functions[f]) {
                    alert("ERROR func: " + f + " " + params);
                    throw ("ERROR func: " + f + " " + params);
                }
                csgs[prefix + seq] = functions[f](params.split(","));
                return prefix + (seq++);
            });
            if (exp != lastexp) {
                console.log("Contune " + exp);
                continue;
            }
            exp = exp.replace(/~(\w+)/g, (a, op1) => {
                csgs[op1] = csgs[op1] || new CSGObject(toCSGPolygons(mqdocument.getObjectByName(op1)));
                csgs[prefix + seq] = csgs[op1].inverse();
                return prefix + (seq++);
            });
            exp = exp.replace(/(\w+)([+\-&|])(\w+)/, (a, op1, op, op2) => {
                csgs[op1] = csgs[op1] || new CSGObject(toCSGPolygons(mqdocument.getObjectByName(op1)));
                csgs[op2] = csgs[op2] || new CSGObject(toCSGPolygons(mqdocument.getObjectByName(op2)));
                if (op == "+" || op == "|") {
                    csgs[prefix + seq] = csgs[op1].union(csgs[op2]);
                } else if (op == "-") {
                    csgs[prefix + seq] = csgs[op2].polygons.length > 0 ? csgs[op1].subtract(csgs[op2]) : csgs[op1];
                } else if (op == "&") {
                    csgs[prefix + seq] = csgs[op1].intersect(csgs[op2]);
                }
                needclean = true;
                return prefix + (seq++);
            });
        } while (exp != lastexp);
        csgs[exp] = csgs[exp] || new CSGObject(toCSGPolygons(mqdocument.getObjectByName(exp)));
        return csgs[exp];
    }
}

class PreviewObject {
    constructor() {
        /** @type {MQObject} */
        this.object = new MQObject();
        this.object.wireframe = true;
        this.target = null;
        this.material = 0;
    }
    setTarget(obj) {
        if (this.target) {
            mqdocument.setDrawProxyObject(this.target, null);
        }
        this.target = obj;
        if (this.target) {
            mqdocument.setDrawProxyObject(this.target, this.object);
        }
    }
    sphere(r) {
        let ctx = this._newPrimitive();
        ctx.sphere(r, 16, 8);
    }
    box(w, h, d) {
        let ctx = this._newPrimitive();
        ctx.box(w, h, d);
    }
    cube(size = 1) {
        let ctx = this._newPrimitive();
        ctx.cube(size);
    }
    cylinder(r, h, s = 20) {
        let ctx = this._newPrimitive();
        ctx.cylinder(r, h, s);
    }
    plane(size = 1) {
        let ctx = this._newPrimitive();
        ctx.plane(size);
    }
    _newPrimitive() {
        this.object.clear();
        return new PrimitiveModeler(this.object, this.material);
    }
}

class NodeObjectWindow {
    constructor() {
        this.form = null;
        /** @type {MQObject} */
        this.object = null;
        this.nodeSpec = {};
        this.previewPrimitive = false;
        this.preview = new PreviewObject();
        this.update();

        mqdocument.addEventListener("_dispose", (ev) => {
            this.dispose();
        });

        mqdocument.addEventListener("OnUpdateObjectList", (ev) => {
            this.update();
        });

        mqdocument.addEventListener("OnObjectModified", (ev) => {
            this.checkForTransformUpdate();
        });
    }
    update() {
        let obj = mqdocument.objects[mqdocument.currentObjectIndex];
        if (obj === this.object) {
            return;
        }
        this.object = obj;
        if (!obj) {
            this.hide();
            return;
        }

        this.load();
    }
    checkForTransformUpdate() {
        if (this.object == null) {
            return;
        }
        this.updateTransformView();
        this.refreshPreview();
    }
    hide() {
        this.form?.close();
        this.form = null;
    }
    load() {
        let obj = this.object;
        let d = mqdocument.getPluginData(SETTING_KEY_PREFIX + obj.id);
        let spec = (d && d != "") ? JSON.parse(d) : { type: TYPE_DEFAULT };
        this.nodeSpec = spec;
        if (this.form) {
            this.form.setContent(this.buildObjectForm(obj, spec));
        } else {
            this.form = createForm(this.buildObjectForm(obj, spec));
            this.form.height = 620; // TODO: fit to content
        }
        setTimeout(() => {
            this.updatePrimitive();
        }, 1);
    }
    updatePrimitive() {
        let nodeSpec = this.nodeSpec;
        let f = this.form;
        let obj = this.object;
        let orgNodeType = nodeSpec.type;
        let orgPrimitiveType = nodeSpec.primitive?.type;
        nodeSpec.type = NODE_TYPES[f.getValue('nodeType')];
        if (nodeSpec.type == TYPE_PRIMITIVE) {
            nodeSpec.primitive = nodeSpec.primitive || {};
            nodeSpec.primitive.type = PRIMITIVE_TYPES[f.getValue('shapeType')];
        } else if (nodeSpec.type == TYPE_CSG) {
            nodeSpec.csg = nodeSpec.csg || {};
            nodeSpec.csg.type = CSG_TYPES[f.getValue('csgType')];
        } else if (nodeSpec.type == TYPE_OBJREF) {
            nodeSpec.objref = nodeSpec.objref || {};
            let obj = mqdocument.objects[+f.getValue('refTarget')];
            nodeSpec.objref.id = obj.id;
            nodeSpec.objref.name = obj.name;
        }
        if (orgNodeType != nodeSpec.type || orgPrimitiveType != nodeSpec.primitive?.type) {
            this.form.setContent(this.buildObjectForm(obj, nodeSpec));
        }
        if (nodeSpec.type == TYPE_PRIMITIVE) {
            let primitive = nodeSpec.primitive;
            primitive.material = +f.getValue('material');
            if (primitive.type == TYPE_CUBE) {
                primitive.size = +f.getValue('size');
            } else if (primitive.type == TYPE_BOX) {
                primitive.width = +f.getValue('width');
                primitive.height = +f.getValue('height');
                primitive.depth = +f.getValue('depth');
            } else if (primitive.type == TYPE_PLANE) {
            } else if (primitive.type == TYPE_CYLINDER) {
                primitive.radius = +f.getValue('radius') || 1;
                primitive.height = +f.getValue('height') || 1;
            } else {
                primitive.radius = +f.getValue('radius') || 1;
            }
        }
        this.refreshPreview();
        this.save();
    }
    refreshPreview() {
        if (!this.previewPrimitive || this.nodeSpec.type != TYPE_PRIMITIVE) {
            this.preview.setTarget(null);
            return;
        }
        let primitive = this.nodeSpec.primitive;
        if (primitive.type == TYPE_CUBE) {
            this.preview.cube(primitive.size);
        } else if (primitive.type == TYPE_BOX) {
            this.preview.box(primitive.width, primitive.height, primitive.depth);
        } else if (primitive.type == TYPE_PLANE) {
            this.preview.plane();
        } else if (primitive.type == TYPE_CYLINDER) {
            this.preview.cylinder(primitive.radius, primitive.height, 20);
        } else {
            this.preview.sphere(primitive.radius);
        }
        this.preview.object.applyTransform(new Matrix4(this.object.globalMatrix));
        this.preview.setTarget(this.object);
    }
    updatePreview() {
        this.previewPrimitive = !!this.form.getValue('preview');
        this.refreshPreview();
    }
    updateTransform() {
        let f = this.form;
        let obj = this.object;
        obj.transform.position = new Vector3(+f.getValue('px'), +f.getValue('py'), +f.getValue('pz'));
        obj.transform.scale = new Vector3(+f.getValue('sx'), +f.getValue('sy'), +f.getValue('sz'));
        obj.transform.rotation = { pitch: +f.getValue('rx'), head: +f.getValue('ry'), bank: +f.getValue('rz') };
    }
    updateTransformView() {
        let f = this.form;
        let obj = this.object;
        let pos = obj.transform.position;
        let scale = obj.transform.scale;
        let rot = obj.transform.rotation;
        f.setValue('px', pos.x);
        f.setValue('py', pos.y);
        f.setValue('pz', pos.z);
        f.setValue('sx', scale.x);
        f.setValue('sy', scale.y);
        f.setValue('sz', scale.z);
        f.setValue('rx', rot.pitch);
        f.setValue('ry', rot.head);
        f.setValue('rz', rot.bank);
    }
    save() {
        mqdocument.setPluginData(SETTING_KEY_PREFIX + this.object.id, JSON.stringify(this.nodeSpec));
    }
    buildObjectForm(obj, params) {
        let nodeTypeId = Math.max(NODE_TYPES.indexOf(params.type), 0);
        let pos = obj.transform.position;
        let scale = obj.transform.scale;
        let rot = obj.transform.rotation;
        let transformChanged = () => {
            this.updateTransform();
        };
        let primitiveChanged = () => {
            this.updatePrimitive();
        };
        /** @type {import("mqwidget").FormSpec} */
        let formSpec = {
            title: "CSG Object",
            items: [
                {
                    type: "group", title: "Object", items: [
                        { type: "hframe", items: ["Name", { id: "name", type: "text", value: obj.name || "", hfill: true }] },
                        {
                            type: "hframe", items: [
                                "T:",
                                { id: "px", type: "number", value: pos.x, onchange: transformChanged },
                                { id: "py", type: "number", value: pos.y, onchange: transformChanged },
                                { id: "pz", type: "number", value: pos.z, onchange: transformChanged }
                            ]
                        },
                        {
                            type: "hframe", items: [
                                "R:",
                                { id: "rx", type: "number", value: rot.pitch, onchange: transformChanged },
                                { id: "ry", type: "number", value: rot.head, onchange: transformChanged },
                                { id: "rz", type: "number", value: rot.bank, onchange: transformChanged }
                            ]
                        },
                        {
                            type: "hframe", items: [
                                "S:",
                                { id: "sx", type: "number", value: scale.x, onchange: transformChanged },
                                { id: "sy", type: "number", value: scale.y, onchange: transformChanged },
                                { id: "sz", type: "number", value: scale.z, onchange: transformChanged }
                            ]
                        },

                    ]
                },
                { type: "hframe", items: ["Type:", { id: "nodeType", type: "combobox", items: NODE_TYPES, value: nodeTypeId, onchange: primitiveChanged }] },
            ]
        }
        if (params.type == TYPE_PRIMITIVE) {
            let shapeTypeId = Math.max(PRIMITIVE_TYPES.indexOf(params.primitive.type), 0);
            let materials = mqdocument.materials.map(m => m.name);
            let primitive = params.primitive;
            formSpec.items.push({ type: "hframe", items: ["Material:", { id: "material", type: "combobox", items: materials, value: primitive.material || 0, onchange: primitiveChanged }] });
            formSpec.items.push({ type: "hframe", items: ["Shape:", { id: "shapeType", type: "combobox", items: PRIMITIVE_TYPES, value: shapeTypeId, onchange: primitiveChanged }] });
            if (primitive.type == TYPE_CUBE) {
                formSpec.items.push({ type: "hframe", items: ["Size", { id: "size", type: "number", value: primitive.size || 1 }] });
            }
            if (primitive.type == TYPE_BOX) {
                formSpec.items.push({
                    type: "hframe", items: [
                        "W", { id: "width", type: "number", value: primitive.width || 1, onchange: primitiveChanged },
                        "H", { id: "height", type: "number", value: primitive.height || 1, onchange: primitiveChanged },
                        "D", { id: "depth", type: "number", value: primitive.depth || 1, onchange: primitiveChanged }]
                });
            }
            if (primitive.type == TYPE_SPHERE) {
                formSpec.items.push({ type: "hframe", items: ["Radius", { id: "radius", type: "number", value: primitive.radius || 1, onchange: primitiveChanged }] });
            }
            if (primitive.type == TYPE_CYLINDER) {
                formSpec.items.push({ type: "hframe", items: ["Radius", { id: "radius", type: "number", value: primitive.radius || 1, onchange: primitiveChanged }] });
                formSpec.items.push({ type: "hframe", items: ["Height", { id: "height", type: "number", value: primitive.height || 1, onchange: primitiveChanged }] });
            }
            formSpec.items.push({ type: "hframe", items: [{ id: "preview", type: "checkbox", label: "Preview", value: this.previewPrimitive, onchange: () => this.updatePreview() }] });
        } else if (params.type == TYPE_CSG) {
            let csgTypeId = Math.max(CSG_TYPES.indexOf(params.csg.type), 0);
            formSpec.items.push({ type: "hframe", items: ["CSG:", { id: "csgType", type: "combobox", items: CSG_TYPES, value: csgTypeId, onchange: primitiveChanged }] });
        } else if (params.type == TYPE_OBJREF) {
            let objNames = mqdocument.objects.map(o => o.name);
            formSpec.items.push({ type: "hframe", items: ["Target:", { id: "refTarget", type: "combobox", items: objNames, value: 0, onchange: primitiveChanged }] });
        }
        formSpec.items.push({ type: "button", value: "Build Mesh", onclick: (v) => { new MeshBuilder().build(obj); } });
        formSpec.items.push({
            type: "button", value: "Hide children", onclick: (v) => {
                obj.visible = true;
                for (let o of obj.getChildren(true)) {
                    o.visible = false;
                }
            }
        });
        return formSpec;
    }
    dispose() {
        this.form?.close();
        this.preview.setTarget(null);
    }
}

let objectWindow = new NodeObjectWindow();
