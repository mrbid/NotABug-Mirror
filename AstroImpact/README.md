# AstroImpact
### Save the martians from their home planet before annihilation!
#### *Kinda.. More like save the planet from inevitable annihilation!*
##### *ok ok ok ... more like ... slow down planet annihilation :'(*

### **FlatHub:** https://flathub.org/apps/com.voxdsp.AstroImpact

### **Official Website:** [AstroImpact.net](https://AstroImpact.net)
### **Video:** [Vimeo.com](https://vimeo.com/836414581)

~~Work In Progress (WIP)~~
**Completed.** *We have left some code for "pods" dormant [[assets here]](assets/old_assets.c) and code in the [[pre_declutter_main]](https://notabug.org/AstroImpact/AstroImpact/src/pre_declutter_main) branch, it was going to be a feature that little escape pods pop out of the planet and that you could pick them up and drop them at a safe zone to evacuate the planet, but we have little interest investing time in that direction now.*

![Screenshot of the AstroImpact game](https://us.v-cdn.net/6030874/uploads/editor/aa/ubn8tqbf6irz.png)

## How do you play it then?

Well you have to fly around in your UFO, preferably with friends, crashing into approaching asteroids to stop them from hitting the planet surface.

* **First Tier:** The longer you can prevent the first hit on the planet.
* **Second Tier:** The longer you can prevent the planet from being annihilated!
* **Third Tier:** How many asteroids did you and your friends collectively destroy?

Just make sure you all start on the same epoch! The first person to create the epoch sets the number of approaching asteroids in each wave.

## Make/Compile
**Standalone Binary**
```
sudo apt install libglfw3 libglfw3-dev
git clone https://notabug.org/AstroImpact/AstroImpact
cd AstroImpact
make
./bin/fat
```
**Installer (.deb)**
```
sudo apt install libglfw3 libglfw3-dev
git clone https://notabug.org/AstroImpact/AstroImpact
cd AstroImpact
make deb
sudo dpkg -i bin/fat.deb
```

## Key/Mouse Mappings
- Use W,A,S,D,Q,E,SPACE & LEFT SHIFT to move around.
- L-CTRL / Right Click to Brake.
- L-ALT / Mouse 3/4 Click to Instant Brake.
- R = Toggle auto-tilt/roll around planet.
- Q+E to stop rolling.
- Escape / Left Click to free mouse focus.
- F = FPS to console.

## Config File
`(fat.cfg) or (.config/fat.cfg) or (~/.config/fat.cfg)`
```
NUM_ASTEROIDS 1337
MSAA 16
AUTOROLL 1
CENTER_WINDOW 1
CONTROLS 0
LOOKSENS 10
ROLLSENS 10
```
The CONTROLS setting in the config file allows you to swap between roll and yaw for mouse x:l=>r.

## Links
- https://notabug.org/AstroImpact/AstroImpact
- https://forum.linuxfoundation.org/discussion/862942/astroimpact-the-foss-space-game
- http://astroimpact.net

## Attributions

Any assets not attributed here have been created specifically for this project, any assets made for this project automatically share the same license as the project itself. Some textures have been generated using [Stable Diffusion](https://en.wikipedia.org/wiki/Stable_Diffusion).

#### Planet
https://www.solarsystemscope.com/textures/
- https://www.solarsystemscope.com/textures/download/4k_eris_fictional.jpg
- https://www.solarsystemscope.com/textures/download/8k_venus_surface.jpg

#### Asteroids
- https://img.freepik.com/free-photo/black-white-details-moon-texture-concept_23-2149535760.jpg?w=2000
- https://img.freepik.com/free-photo/black-white-details-moon-texture-concept_23-2149535764.jpg?w=2000
- https://static9.depositphotos.com/1011845/1198/i/600/depositphotos_11983420-stock-photo-moon-ground-semalss-texture.jpg
- https://www.reddit.com/r/Unity3D/comments/6sdq7s/i_just_made_a_seamless_lunar_surface_texture_you/
- https://i.imgur.com/NPJfZXGl.png
- https://i.pinimg.com/originals/19/33/0b/19330beb4d18d4baf9a522a3649210a5.jpg
- https://as2.ftcdn.net/v2/jpg/00/50/64/23/1000_F_50642395_EzECINojo0khiXKgl33LOnoNwNBIhSNm.jpg
- https://filterforge.com/filters/13302.jpg
- https://filterforge.com/filters/13302-v1.jpg
- https://filterforge.com/filters/1134.jpg
- https://filterforge.com/filters/214.jpg
- https://filterforge.com/filters/1306.jpg
- https://www.deviantart.com/hhh316/art/Seamless-Fire-Texture-333891187

#### UFO
- https://www.turbosquid.com/3d-models/sci-fi-ufo-2-3d-model/1062607# _(This was provided from the Author James directly over email ([jamesrender3d@gmail.com](mailto:jamesrender3d@gmail.com)) and NOT purchased from Turbosquid under the Turbosquid license that does not allow use of assets in open-source projects, a custom license was agree'd over email with James where he agree'd we could use it in our open-source project and a payment was made to him based on this agreement this does not give anyone else the right to use this 3D asset (model or textures) without his permission or payment and this asset is not licensed under GPL)_