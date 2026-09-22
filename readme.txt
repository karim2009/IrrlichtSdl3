Irrlicht Engine 2.0.0 ? Vortex3D Edition (SDL3 + OpenGL + Wayland)
Moteur 3D C++17 modernisé pour Linux/Ubuntu. Cette version 2.0.0 remplace le sous-système d'affichage legacy d'Irrlicht par une intégration SDL3 compatible Wayland / X11, embarque des shaders GLSL personnalisés et intègre nativement la suite de plugins Vortex3D dans la bibliothèque statique libIrrlicht.a.

1. Nouveautés de la Version 2.0.0
Backend SDL3 & Wayland/X11 : Gestion moderne du contexte de fenêtrage et des entrées via SDL3.

Support binaire Urho3D / Turso3D (.mdl) : Intégration du parser natif LoadMdl (format UMDL).

Support Assimp universel : Importation à la volée de maillages statiques et animés (.gltf, .fbx, .obj, etc.) via LoadModel et LoadActor.

Modules Vortex3D intégrés : Compilés directement au c?ur de libIrrlicht.a (InitScene, Camera, Shaders, LoadActor, LoadMdl, LoadModel).

Tooling inclus : Outil autonome vortex-mdl-exporter pour convertir n'importe quel format Assimp en fichier binaire .mdl.

2. Structure des Modules Intégrés (libIrrlicht.a)
Les headers sont situés dans include/ et les implémentations dans source/Irrlicht/ :

InitScene : Initialisation du device SDL3/Wayland, contrôle du frame buffer et de la couleur de fond (SetBackgroundColor, BeginFrame).

Camera : Caméra orbitale configurable avec ciblage et gestion du curseur.

LoadMdl : Parser/loader haute performance pour les modèles binaire statiques .mdl d'Urho3D.

LoadActor : Wrapper de chargement et gestion des acteurs animés/squelettiques (GLTF/FBX) via Assimp.

LoadModel : Loader générique de maillages via Assimp.

Shaders : Gestionnaire de pipelines et effets GLSL (Fresnel, effet glacé, etc.).

3. Prérequis
Système d'exploitation : Linux (Ubuntu 22.04 / 24.04 / Kubuntu)

Compilateur : GCC ou Clang compatible C++17

Bibliothèques requises :

SDL3

OpenGL / GLX

libassimp-dev

libX11-dev / libXxf86vm-dev

CMake (>= 3.16) & Make

4. Compilation de la Bibliothèque libIrrlicht.a
Pour recompiler la bibliothèque statique d'Irrlicht avec tous les modules embarqués :

Bash
cd source/Irrlicht

# Nettoyage des objets précédents
make clean

# Compilation multithreadée de libIrrlicht.a
make -j$(nproc)
La bibliothèque générée sera disponible sous source/Irrlicht/libIrrlicht.a.

5. Compilation de l'Application & des Outils (CMake)
Le projet principal Vortex3D utilise CMake pour lier libIrrlicht.a et générer les exécutables.

Bash
cd /chemin/vers/votre_projet
mkdir -p build && cd build

# Génération des fichiers Makefiles
cmake ..

# Compilation de l'application principale
make App

# Compilation de l'outil d'exportation MDL
make vortex-mdl-exporter
6. Utilisation de l'Outil vortex-mdl-exporter
Cet outil convertit n'importe quel modèle 3D (.obj, .fbx, .gltf) en modèle binaire .mdl optimisé pour le moteur :

Bash
./vortex-mdl-exporter /chemin/vers/mon_modele.obj /chemin/vers/Media/Models/Cube.mdl
7. Exemple de Code (main.cpp)
Voici comment initialiser le moteur et afficher un modèle statique .mdl et un acteur .gltf :

C++
#include "InitScene.h"
#include "Camera.h"
#include "LoadMdl.h"
#include "LoadActor.h"
#include "Shaders.h"

using namespace irr;

int main() {
    // 1. Initialisation du device SDL3 (Wayland / X11)
    IrrlichtDevice* device = Vortex3D::InitScene(1024, 768, "Vortex3D Engine 2.0.0", false);
    if (!device) return 1;

    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();

    Vortex3D::SetBackgroundColor(255, 100, 101, 140);

    // 2. Configuration de la caméra orbitale
    Vortex3D::Camera mainCamera(device);
    mainCamera.SetCameraLook(0.0f, 1.0f, 0.0f);
    mainCamera.SetCamera(0.0f, 2.0f, -5.0f);
    mainCamera.SetOrbitalCamera(true);

    // 3. Modèle binaire .mdl (Chargement synchrone)
    Vortex3D::LoadMdl* staticMdl = new Vortex3D::LoadMdl(smgr, driver);
    if (staticMdl->Load("Media/Models/Cube.mdl")) {
        staticMdl->LoadTexture("Media/Models/Mushroom.dds");
        staticMdl->SetPosition(-1.5f, 0.0f, 0.0f);
    }

    // 4. Modèle d'Acteur Assimp (.gltf)
    Vortex3D::LoadActor* alienActor = new Vortex3D::LoadActor(smgr, driver);
    if (alienActor->Load("Media/Models/Alien.gltf")) {
        alienActor->LoadTexture("Media/Models/Atlas_Monsters.png");
        alienActor->SetPosition(1.5f, 0.0f, 0.0f);
    }

    // 5. Boucle principale de rendu
    while (device->run()) {
        Vortex3D::BeginFrame(driver);
        smgr->drawAll();
        driver->endScene();
    }

    // Nettoyage
    delete staticMdl;
    delete alienActor;
    smgr->clear();
    device->drop();

    return 0;
}
