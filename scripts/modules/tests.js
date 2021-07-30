// simple test runner.
class Assert {
    constructor() {
        this.count = 0;
    }
    reset() {
        this.count = 0;
    }
    assert(v, message) {
        if (v !== true) throw "FAILED: " + message;
        this.count++;
    }
    equals(expect, value, message) {
        this.assert(expect === value, "" + value + " != " + expect + "(expect) msg:" + message);
    }
    notNull(value, message) {
        this.assert(value != null, "" + value + " is not null msg:" + message);
    }
    isNull(value, message) {
        this.assert(value === null, "" + value + " is null msg:" + message);
    }
    throws(t, b, message) {
        let ok = false;
        try { b(); } catch (e) { ok = (e instanceof t); }
        this.assert(ok, t.name + " not throws. " + message);
    }
}

const assert = new Assert();
const testState = {
    count: 0,
    success: 0,
    onFailed(t, e) {
        console.log("failed. assert:" + assert.count + " " + e);
    },
    onSuccess(t) {
        if (assert.count) {
            console.log(" ok. " + assert.count + " assert(s).");
            assert.reset();
        }
    }
};

/**
 * @param {string} name 
 * @param {(state: object)=>void} testFunc 
 */
function test(name, testFunc) {
    console.log(name);
    try {
        testFunc(testState);
        testState.onSuccess();
        testState.success++;
    } catch (e) {
        testState.onFailed(testState, e);
    } finally {
        testState.count++;
    }
}

export { assert, test }
