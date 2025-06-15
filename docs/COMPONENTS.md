# Component definition files

Components represent objects that manipulate an Actor, such as a physics implementation or a scripting instance. They are implemented as SceneNodeAnimators

## .component file structure

Component definition files are XML documents that define a single component.

Example

```
<?xml version="1.0" encoding="UTF-8"?>
<component name="Collider">
    <float softness="0.1" />
</component>
```

## Elements
Element and Attribute names are case-sensitive.

### Root
The root element \<component> defines the Component's basic details

- name: the name of the component

### Properties
Properties define Component's specific data elements. The available data types for properties are:

- int: integer

  e.g. \<int Health="100" />

- string: UTF-8 encoded text

  e.g. \<string Mesh="package.zip:mesh/example.obj" />

- float: floating point number

  e.g. \<float RunSpeed="1.25" />

- vec2: 2D vector

  e.g. \<vec2 Fov="0.7 0.5" />

- vec3: 3D vector

  e.g. \<vec3 AreaOfEffect="10.0 30.5 11.1" \>

