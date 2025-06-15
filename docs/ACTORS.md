# Actor definition files

Actors are entities that are part of an Entity-Component-System (ECS), and be controlled and interacted with by the player.

Examples of an Actor could be a NPC, a particle emitter, or a movable object.

## .actor file format

Actor definition files are XML documents that define one Actor.

Example

```
<?xml version="1.0" encoding="UTF-8"?>
<actor name="Sydney" category="NPC" type="Model">
  <properties>
    <int Health="100"/>
    <string Texture="demo.zip:textures/sydney.bmp"/>
    <string Mesh="demo.zip:models/sydney.md2"/>
  </properties>
  <components>
    <component name="Collider">
      <float softness="0.1" />
    </component>
    <component name="Solid">
      <vec3 mass="1.0 1.0 1.0" />
    </component>
  </components>
</actor>
```

## Elements
Element and Attribute names are case-sensitive.

### Root
The root element \<actor> defines the Actor's basic details

- name: the display name of the Actor
- category: the parent leaf in the Actor Browser tree
- type: one of Model, Emitter, Custom

### Properties
Properties define Actor specific data elements. The available data types for properties are:

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

### Components

Components are populated based on [.component](COMPONENTS.md) file definitions.
