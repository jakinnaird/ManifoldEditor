# Prefab static mesh definition files

Static meshes are models that behave like standard geometry, but are created in a separate modeling package.

## .prefab file structure

Prefab files are XML that defines a single static mesh per file, and up to 4 textures.

```
<?xml version="1.0" encoding="UTF-8"?>
<prefab name="sydney">
  <mesh>demo.zip:models/sydney.md2</mesh>
  <texture0>demo.zip:textures/sydney.bmp</texture0>
  <texture1 />
  <texture2 />
  <texture3 />
</prefab>
```

The tag and attribute names are case-sensitive. The prefab name attribute must be set, and does not need to be unique, as it is used for display purposes only. The mesh and texture node contents can be either a package URI as shown, or a fully qualified or relative path to the files needed. Relative paths must be in relation to the ManifoldEditor binary path.

Prefab files can also be added to MPK or ZIP archives at any level of the archive. If you are bundling .prefab files and the associated mesh and texture files, the recommended archive layout is:

```
+ models/
|-- mesh1.obj
|-- mesh2.obj
+ textures/
|-- mesh1tex.bmp
|-- mesh2tex.png
mesh1.prefab
mesh2.prefab
```
