"use strict";
// Invert faces in selected object.

document.compact();

var selectedObjects = document.objects.filter((obj) => { return obj.selected; } );

selectedObjects.forEach((obj) => {
	obj.faces.forEach((f,idx) => {
		f.invert();
	});
});

