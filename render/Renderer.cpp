#include <vector>
#include <algorithm>
#include "../Engine.h"
#include "Camera.h"
#include "Renderer.h"
#include "ViewDescriptor.h"
#include "RenderTarget.h"
#include "RenderTargetCube.h"
#include "RenderTarget2D.h"
#include "../math/Matrix4x4.h"
#include "../render/BaseRenderableContainer.h"
#include "../render/MeshRenderer.h"
#include "../render/RenderableContainer.h"
#include "../scene/Scene.h"
#include "../scene/Skybox.h"
#include "../image/TextureAttr.h"
#include "../image/Texture.h"
#include "../image/Texture2D.h"
#include "../material/DepthOnlyMaterial.h"
#include "../material/BasicColoredMaterial.h"
#include "../material/DistanceOnlyMaterial.h"
#include "../material/TonemapMaterial.h"
#include "../material/IrradianceRendererMaterial.h"
#include "../material/SpecularIBLPreFilteredRendererMaterial.h"
#include "../material/SpecularIBLBRDFRendererMaterial.h"
#include "../math/Matrix4x4.h"
#include "../math/Quaternion.h"
#include "../light/PointLight.h"
#include "../light/AmbientIBLLight.h"
#include "../geometry/Mesh.h"
#include "ReflectionProbe.h"


namespace Core {

    Renderer::Renderer() {
        
    }

    Renderer::~Renderer() {
        if (this->perspectiveShadowMapCameraObject.isValid()) Engine::safeReleaseObject(this->perspectiveShadowMapCameraObject);
        if (this->orthoShadowMapCameraObject.isValid()) Engine::safeReleaseObject(this->orthoShadowMapCameraObject);
    }

    Bool Renderer::init() {
        if (!this->depthMaterial.isValid()) {
            this->depthMaterial = Engine::instance()->createMaterial<DepthOnlyMaterial>();
            this->depthMaterial->setLit(false);
        }
        if (!this->distanceMaterial.isValid()) {
            this->distanceMaterial = Engine::instance()->createMaterial<DistanceOnlyMaterial>();
            this->distanceMaterial->setLit(false);
        }
        if (!this->tonemapMaterial.isValid()) {
            this->tonemapMaterial = Engine::instance()->createMaterial<TonemapMaterial>();
            this->tonemapMaterial->setExposure(1.0f);
            this->tonemapMaterial->setLit(false);
        }
        return true;
    }

    void Renderer::renderScene(WeakPointer<Scene> scene, WeakPointer<Material> overrideMaterial) {
        this->renderScene(scene->getRoot(), overrideMaterial);
    }

    void Renderer::renderScene(WeakPointer<Object3D> rootObject, WeakPointer<Material> overrideMaterial) {
        static std::vector<WeakPointer<Object3D>> objectList;
        static std::vector<WeakPointer<Camera>> cameraList;
        static std::vector<WeakPointer<Light>> lightList;
        static std::vector<WeakPointer<Light>> nonIBLLightList;
        static std::vector<WeakPointer<ReflectionProbe>> reflectionProbeList;
        static std::vector<WeakPointer<Object3D>> emptyObjectList;
        objectList.resize(0);
        cameraList.resize(0);
        lightList.resize(0);
        nonIBLLightList.resize(0);
        reflectionProbeList.resize(0);

        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        this->processScene(rootObject, objectList);

        for (WeakPointer<Object3D> object : objectList) {
            for (SceneObjectIterator<Object3DComponent> compItr = object->beginIterateComponents(); compItr != object->endIterateComponents(); ++compItr) {
                WeakPointer<Object3DComponent> comp = (*compItr);
                WeakPointer<Camera> camera = WeakPointer<Object3DComponent>::dynamicPointerCast<Camera>(comp);
                if (camera.isValid() && camera->isActive()) {
                    cameraList.push_back(camera);
                    continue;
                }
                WeakPointer<ReflectionProbe> reflectionProbe = WeakPointer<Object3DComponent>::dynamicPointerCast<ReflectionProbe>(comp);
                if (reflectionProbe.isValid() && reflectionProbe->isActive()) {
                    reflectionProbeList.push_back(reflectionProbe);
                    continue;
                }
            }
            for (SceneObjectIterator<Object3DComponent> compItr = object->beginIterateComponents(); compItr != object->endIterateComponents(); ++compItr) {
                WeakPointer<Object3DComponent> comp = (*compItr);
                WeakPointer<Light> light = WeakPointer<Object3DComponent>::dynamicPointerCast<Light>(comp);
                if (light.isValid() && light->isActive()) {
                    if (light->getType() == LightType::AmbientIBL) {
                        if (reflectionProbeList.size() > 0) {
                            WeakPointer<AmbientIBLLight> ambientIBLlight = WeakPointer<Object3DComponent>::dynamicPointerCast<AmbientIBLLight>(comp);
                            
                            WeakPointer<CubeTexture> irradianceMap = WeakPointer<Texture>::dynamicPointerCast<CubeTexture>(reflectionProbeList[0]->getIrradianceMap()->getColorTexture());
                            ambientIBLlight->setIrradianceMap(irradianceMap);
                            
                            WeakPointer<CubeTexture> specularIBLPreFilteredMap = WeakPointer<Texture>::dynamicPointerCast<CubeTexture>(reflectionProbeList[0]->getSpecularIBLPreFilteredMap()->getColorTexture());
                            ambientIBLlight->setSpecularIBLPreFilteredMap(specularIBLPreFilteredMap);
                            
                            WeakPointer<Texture2D> specularIBLBRDFMap = WeakPointer<Texture>::dynamicPointerCast<Texture2D>(reflectionProbeList[0]->getSpecularIBLBRDFMap()->getColorTexture());
                            ambientIBLlight->setSpecularIBLBRDFMap(specularIBLBRDFMap);
                        }
                        else continue;
                    }
                    else {
                        nonIBLLightList.push_back(light);
                    }
                    lightList.push_back(light);
                    continue;
                }
            }
        }

        std::sort(lightList.begin(), lightList.end(), Renderer::compareLights);
        std::sort(nonIBLLightList.begin(), nonIBLLightList.end(), Renderer::compareLights);
        
        this->renderShadowMaps(lightList, LightType::Point, objectList);
        for (auto camera : cameraList) {
            this->renderShadowMaps(lightList, LightType::Directional, objectList, camera);
        }

        for (auto reflectionProbe : reflectionProbeList) {
            if (reflectionProbe->getNeedsFullUpdate() || reflectionProbe->getNeedsSpecularUpdate()) {
                Bool specularOnly = !reflectionProbe->getNeedsFullUpdate();
                this->renderReflectionProbe(reflectionProbe, specularOnly, objectList, nonIBLLightList);
                if (specularOnly) reflectionProbe->setNeedsSpecularUpdate(false);
                else reflectionProbe->setNeedsFullUpdate(false);
            }
        }

        for (auto camera : cameraList) {
            this->render(camera, objectList, lightList, overrideMaterial, true);
        }
    }

    void Renderer::renderObjectBasic(WeakPointer<Object3D> rootObject, WeakPointer<Camera> camera,
                                     WeakPointer<Material> overrideMaterial, Bool matchPhysicalPropertiesWithLighting) {
        static std::vector<WeakPointer<Object3D>> objectList;
        objectList.resize(0);

        Matrix4x4 baseTransformation;
        rootObject->getTransform().getAncestorWorldMatrix(baseTransformation);

        this->processScene(rootObject, objectList, baseTransformation);
        this->render(camera, objectList, overrideMaterial, matchPhysicalPropertiesWithLighting);
    }

    void Renderer::render(WeakPointer<Camera> camera, std::vector<WeakPointer<Object3D>>& objects, 
                          WeakPointer<Material> overrideMaterial, Bool matchPhysicalPropertiesWithLighting) {
        static std::vector<WeakPointer<Light>> lightList;   
        this->render(camera, objects, lightList, overrideMaterial, matchPhysicalPropertiesWithLighting);                 
    }

    void Renderer::render(WeakPointer<Camera> camera, std::vector<WeakPointer<Object3D>>& objects, std::vector<WeakPointer<Light>>& lights,
                          WeakPointer<Material> overrideMaterial, Bool matchPhysicalPropertiesWithLighting) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        WeakPointer<RenderTarget> nextRenderTarget = camera->getRenderTarget();
        if (!nextRenderTarget.isValid()) {
            nextRenderTarget = graphics->getDefaultRenderTarget();
        }

        RenderTargetCube * renderTargetCube = dynamic_cast<RenderTargetCube*>(nextRenderTarget.get());
        if (renderTargetCube != nullptr) {
            this->renderCube(camera, objects, lights, overrideMaterial, matchPhysicalPropertiesWithLighting);
        }
        else {
            this->renderStandard(camera, objects, lights, overrideMaterial, matchPhysicalPropertiesWithLighting);
        }
    }

    void Renderer::renderStandard(WeakPointer<Camera> camera, std::vector<WeakPointer<Object3D>>& objects,
                                  std::vector<WeakPointer<Light>>& lights, WeakPointer<Material> overrideMaterial,
                                  Bool matchPhysicalPropertiesWithLighting) {
        ViewDescriptor viewDescriptor;
        this->getViewDescriptorForCamera(camera, viewDescriptor);
        viewDescriptor.overrideMaterial = overrideMaterial;
        render(viewDescriptor, objects, lights, matchPhysicalPropertiesWithLighting);
    }

    void Renderer::renderCube(WeakPointer<Camera> camera, std::vector<WeakPointer<Object3D>>& objects,
                    std::vector<WeakPointer<Light>>& lights, WeakPointer<Material> overrideMaterial,
                    Bool matchPhysicalPropertiesWithLighting) {

        static bool initialized = false;
        static Matrix4x4 forward;
        static Matrix4x4 left;
        static Matrix4x4 right;
        static Matrix4x4 up;
        static Matrix4x4 down;
        static Matrix4x4 backward;
        static std::vector<Matrix4x4> orientations;
        if (!initialized) {
            initialized = true;
            forward.lookAt(Vector3r::Zero, Vector3r::Backward, Vector3r::Down);
            backward.lookAt(Vector3r::Zero, Vector3r::Forward, Vector3r::Down);
            up.lookAt(Vector3r::Zero, Vector3r::Up, Vector3r::Backward);
            down.lookAt(Vector3r::Zero, Vector3r::Down, Vector3r::Forward);
            left.lookAt(Vector3r::Zero, Vector3r::Left, Vector3r::Down);
            right.lookAt(Vector3r::Zero, Vector3r::Right, Vector3r::Down);
            orientations.push_back(forward);
            orientations.push_back(backward);
            orientations.push_back(up);
            orientations.push_back(down);
            orientations.push_back(left);
            orientations.push_back(right);
        }

        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        ViewDescriptor baseViewDescriptor;
        this->getViewDescriptorForCamera(camera, baseViewDescriptor);
        for (unsigned int i = 0; i < 6; i++) {
            ViewDescriptor viewDescriptor = baseViewDescriptor;
            Matrix4x4 cameraTransform = camera->getOwner()->getTransform().getWorldMatrix();
            cameraTransform.multiply(orientations[i]);
            this->getViewDescriptorTransformations(cameraTransform, camera->getProjectionMatrix(),
                                                   camera->getAutoClearRenderBuffers(), viewDescriptor);
            viewDescriptor.overrideMaterial = overrideMaterial;
            viewDescriptor.cubeFace = i;
            render(viewDescriptor, objects, lights, matchPhysicalPropertiesWithLighting);
        }
    }

    void Renderer::render(ViewDescriptor& viewDescriptor, std::vector<WeakPointer<Object3D>>& objectList, 
                          std::vector<WeakPointer<Light>>& lightList, Bool matchPhysicalPropertiesWithLighting) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        WeakPointer<RenderTarget> currentRenderTarget = graphics->getCurrentRenderTarget();

        WeakPointer<RenderTarget> nextRenderTarget = viewDescriptor.indirectHDREnabled ? viewDescriptor.hdrRenderTarget : viewDescriptor.renderTarget;
        graphics->activateRenderTarget(nextRenderTarget);       
        this->setViewportAndMipLevelForRenderTarget(nextRenderTarget, viewDescriptor.cubeFace);

        this->clearActiveRenderTarget(viewDescriptor);

        this->renderSkybox(viewDescriptor);
        for (auto object : objectList) {
            this->renderObjectDirect(object, viewDescriptor, lightList, matchPhysicalPropertiesWithLighting);
        }

        if (viewDescriptor.indirectHDREnabled) {
            this->tonemapMaterial->setToneMapType(viewDescriptor.hdrToneMapType);
            this->tonemapMaterial->setExposure(viewDescriptor.hdrExposure);
            this->tonemapMaterial->setGamma(viewDescriptor.hdrGamma);
            graphics->blit(viewDescriptor.hdrRenderTarget, viewDescriptor.renderTarget, viewDescriptor.cubeFace, this->tonemapMaterial, true);
        }

        graphics->activateRenderTarget(currentRenderTarget);
        this->setViewportAndMipLevelForRenderTarget(currentRenderTarget, -1);
    }

    void Renderer::clearActiveRenderTarget(ViewDescriptor& viewDescriptor) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        Bool clearColorBuffer = IntMaskUtil::isBitSetForMask(viewDescriptor.clearRenderBuffers, (UInt32)RenderBufferType::Color);
        Bool clearDepthBuffer = IntMaskUtil::isBitSetForMask(viewDescriptor.clearRenderBuffers, (UInt32)RenderBufferType::Depth);
        Bool clearStencilBuffer = IntMaskUtil::isBitSetForMask(viewDescriptor.clearRenderBuffers, (UInt32)RenderBufferType::Stencil);
        graphics->clearActiveRenderTarget(clearColorBuffer, clearDepthBuffer, clearStencilBuffer); 

    }

    void Renderer::setViewportAndMipLevelForRenderTarget(WeakPointer<RenderTarget> renderTarget, Int16 cubeFace) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        UInt32 targetMipLevel = renderTarget->getMipLevel();
        if (cubeFace >= 0)
            graphics->activateCubeRenderTargetSide((CubeTextureSide)cubeFace, targetMipLevel);
        else
            graphics->activateRenderTarget2DMipLevel(targetMipLevel);
        Vector4u mipLevelScaledViewport = renderTarget->getViewportForMipLevel(targetMipLevel);
        graphics->setViewport(mipLevelScaledViewport.x, mipLevelScaledViewport.y, mipLevelScaledViewport.z, mipLevelScaledViewport.w);

    }

    void Renderer::renderSkybox(ViewDescriptor& viewDescriptor) {
        if (viewDescriptor.skybox != nullptr) {
            WeakPointer<BaseObjectRenderer> objectRenderer = viewDescriptor.skybox->getSkyboxObject()->getBaseRenderer();
            if (objectRenderer) {
                ViewDescriptor skyboxView = viewDescriptor;
                std::vector<WeakPointer<Light>> dummyLights;
                skyboxView.viewMatrix.setTranslation(0.0f, 0.0f, 0.0f);
                skyboxView.viewInverseMatrix.copy(skyboxView.viewMatrix);
                skyboxView.viewInverseMatrix.invert();
                skyboxView.viewInverseTransposeMatrix.copy(skyboxView.viewInverseMatrix);
                skyboxView.viewInverseTransposeMatrix.transpose();
                objectRenderer->forwardRender(skyboxView, dummyLights, true);
            }
        }
    }

    void Renderer::renderObjectDirect(WeakPointer<Object3D> object, WeakPointer<Camera> camera, WeakPointer<Material> overrideMaterial,
                                      Bool matchPhysicalPropertiesWithLighting) {
        static std::vector<WeakPointer<Light>> lightList;
        this->renderObjectDirect(object, camera, lightList, overrideMaterial, matchPhysicalPropertiesWithLighting);
    }

    void Renderer::renderObjectDirect(WeakPointer<Object3D> object, WeakPointer<Camera> camera,
                                     std::vector<WeakPointer<Light>>& lightList, WeakPointer<Material> overrideMaterial,
                                     Bool matchPhysicalPropertiesWithLighting) {
        ViewDescriptor viewDescriptor;
        this->getViewDescriptorForCamera(camera, viewDescriptor);
        viewDescriptor.overrideMaterial = overrideMaterial;
        this->renderObjectDirect(object, viewDescriptor, lightList, matchPhysicalPropertiesWithLighting);
    }

    void Renderer::renderObjectDirect(WeakPointer<Object3D> object, ViewDescriptor& viewDescriptor,
                            std::vector<WeakPointer<Light>>& lightList, Bool matchPhysicalPropertiesWithLighting) {
        std::shared_ptr<Object3D> objectShared = object.lock();
        std::shared_ptr<BaseRenderableContainer> containerPtr = std::dynamic_pointer_cast<BaseRenderableContainer>(objectShared);
        if (containerPtr) {
            WeakPointer<BaseObjectRenderer> objectRenderer = containerPtr->getBaseRenderer();
            if (objectRenderer) {
                objectRenderer->forwardRender(viewDescriptor, lightList, matchPhysicalPropertiesWithLighting);
            }
        }
    }

    void Renderer::renderShadowMaps(std::vector<WeakPointer<Light>>& lights, LightType lightType,
                                    std::vector<WeakPointer<Object3D>>& objects, WeakPointer<Camera> renderCamera) {

        static std::vector<WeakPointer<Object3D>> toRender;
        if (!this->perspectiveShadowMapCamera.isValid()) {
            this->perspectiveShadowMapCameraObject = Engine::instance()->createObject3D();
            this->perspectiveShadowMapCamera = Engine::instance()->createPerspectiveCamera(perspectiveShadowMapCameraObject, Math::PI / 2.0f, 1.0f, PointLight::NearPlane, PointLight::FarPlane);
            this->orthoShadowMapCameraObject = Engine::instance()->createObject3D();
            this->orthoShadowMapCamera = Engine::instance()->createOrthographicCamera(orthoShadowMapCameraObject, 1.0f, -1.0f, -1.0f, 1.0f, PointLight::NearPlane, PointLight::FarPlane);
        }

        toRender.resize(0);
        for (UInt32 i = 0; i < objects.size(); i++) {
            WeakPointer<Object3D> object = objects[i];
            std::shared_ptr<Object3D> objectShared = object.lock();
            std::shared_ptr<BaseRenderableContainer> containerPtr = std::dynamic_pointer_cast<BaseRenderableContainer>(objectShared);
            if (containerPtr) {
                WeakPointer<BaseObjectRenderer> objectRenderer = containerPtr->getBaseRenderer();
                if (objectRenderer && objectRenderer->castsShadows()) {
                    toRender.push_back(object);
                }
            }
        }

        std::vector<WeakPointer<Light>> dummyLights;
        for (auto light: lights) {
            LightType clightType = light->getType();
            if (clightType == lightType && isShadowCastingCapableLight(light)) {
                switch(lightType) {
                    case LightType::Point:
                    {
                        WeakPointer<PointLight> pointLight = WeakPointer<Light>::dynamicPointerCast<PointLight>(light);
                        if (pointLight->getShadowsEnabled()) {
                            WeakPointer<RenderTarget> shadowMapRenderTarget = pointLight->getShadowMap();
                            WeakPointer<Object3D> lightObject = light->getOwner();
                            Matrix4x4 lightTransform = lightObject->getTransform().getWorldMatrix();
                            this->perspectiveShadowMapCameraObject->getTransform().getWorldMatrix().copy(lightTransform);
                            Vector4u renderTargetDimensions = shadowMapRenderTarget->getViewport();
                            this->perspectiveShadowMapCamera->setRenderTarget(shadowMapRenderTarget);  
                            this->perspectiveShadowMapCamera->setAspectRatioFromDimensions(renderTargetDimensions.z, renderTargetDimensions.w);                     
                            this->render(perspectiveShadowMapCamera, toRender, dummyLights, this->distanceMaterial, true);
                        }
                    }
                    break;
                    case LightType::Directional:
                    {
                        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
                        WeakPointer<DirectionalLight> directionalLight = WeakPointer<Light>::dynamicPointerCast<DirectionalLight>(light);
                        if (directionalLight->getShadowsEnabled()) {
                            std::vector<DirectionalLight::OrthoProjection>& projections = directionalLight->buildProjections(renderCamera);
                            Matrix4x4 viewTrans = directionalLight->getOwner()->getTransform().getWorldMatrix();
                            for (UInt32 i = 0; i < directionalLight->getCascadeCount(); i++) {
                                DirectionalLight::OrthoProjection& proj = projections[i];  
                                this->orthoShadowMapCamera->setDimensions(proj.top, proj.bottom, proj.left, proj.right);        
                                this->orthoShadowMapCamera->setNearAndFar(proj.near, proj.far);

                                ViewDescriptor viewDesc;
                                viewDesc.indirectHDREnabled = false;
                                viewDesc.cubeFace = -1;
                                this->getViewDescriptorTransformations(viewTrans, orthoShadowMapCamera->getProjectionMatrix(),
                                                                       this->orthoShadowMapCamera->getAutoClearRenderBuffers(), viewDesc);
                                viewDesc.overrideMaterial = this->depthMaterial;
                                viewDesc.renderTarget = directionalLight->getShadowMap(i);
                                this->render(viewDesc, toRender, dummyLights, true);
                            }
                        }
                    }
                }
            }
        }
    }
    
    void Renderer::getViewDescriptorForCamera(WeakPointer<Camera> camera, ViewDescriptor& viewDescriptor) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        WeakPointer<RenderTarget> cameraRenderTarget = camera->getRenderTarget();
        if (!cameraRenderTarget.isValid()) {
            cameraRenderTarget = graphics->getDefaultRenderTarget();
        }
        if (camera->isHDREnabled() && !cameraRenderTarget->isHDRCapable()) {
            Vector2u targetSize = cameraRenderTarget->getSize();
            WeakPointer<RenderTarget2D> cameraHDRRenderTarget = camera->getHDRRenderTarget();
            if (!cameraHDRRenderTarget.isValid() || 
                cameraHDRRenderTarget->getSize().x != targetSize.x ||
                cameraHDRRenderTarget->getSize().y != targetSize.y) {
                camera->buildHDRRenderTarget(targetSize);
            }
            viewDescriptor.renderTarget = cameraRenderTarget;
            viewDescriptor.hdrRenderTarget = camera->getHDRRenderTarget();
            viewDescriptor.indirectHDREnabled = true;
            viewDescriptor.hdrToneMapType = camera->getHDRToneMapType();
        }
        else {
            viewDescriptor.indirectHDREnabled = false;
            viewDescriptor.hdrRenderTarget = WeakPointer<RenderTarget2D>::nullPtr();
            viewDescriptor.renderTarget = cameraRenderTarget;
        }
        viewDescriptor.hdrExposure = camera->getHDRExposure();
        viewDescriptor.hdrGamma = camera->getHDRGamma();
        viewDescriptor.skybox = camera->isSkyboxEnabled() ? &camera->getSkybox() : nullptr;
        this->getViewDescriptorTransformations(camera->getOwner()->getTransform().getWorldMatrix(),
                                camera->getProjectionMatrix(), camera->getAutoClearRenderBuffers(), viewDescriptor);
        viewDescriptor.cameraPosition.set(0.0f, 0.0f, 0.0f);
        viewDescriptor.cubeFace = -1;
        viewDescriptor.viewMatrix.transform(viewDescriptor.cameraPosition);
       
    }

    void Renderer::getViewDescriptorTransformations(const Matrix4x4& worldMatrix, const Matrix4x4& projectionMatrix,
                                     IntMask clearBuffers, ViewDescriptor& viewDescriptor) {
        viewDescriptor.projectionMatrix.copy(projectionMatrix);
        viewDescriptor.viewMatrix.copy(worldMatrix);
        viewDescriptor.viewInverseMatrix.copy(viewDescriptor.viewMatrix);
        viewDescriptor.viewInverseMatrix.invert();
        viewDescriptor.viewInverseTransposeMatrix.copy(viewDescriptor.viewInverseMatrix);
        viewDescriptor.viewInverseTransposeMatrix.transpose();
        viewDescriptor.clearRenderBuffers = clearBuffers;
    }

    void Renderer::processScene(WeakPointer<Scene> scene, std::vector<WeakPointer<Object3D>>& outObjects) {
        processScene(scene->getRoot(), outObjects);
    }

    void Renderer::processScene(WeakPointer<Object3D> object, std::vector<WeakPointer<Object3D>>& outObjects) {
        Matrix4x4 rootTransform;
        processScene(object, outObjects, rootTransform);
    }

    void Renderer::processScene(WeakPointer<Object3D> object, std::vector<WeakPointer<Object3D>>& outObjects, const Matrix4x4& curTransform) {

        if (!object->isActive()) return;
        Matrix4x4 nextTransform = curTransform;
        Transform& objTransform = object->getTransform();
        nextTransform.multiply(objTransform.getLocalMatrix());
        objTransform.getWorldMatrix().copy(nextTransform);
        Matrix4x4& inverseWorld = objTransform.getInverseWorldMatrix();
        inverseWorld.copy(nextTransform);
        inverseWorld.invert();
        outObjects.push_back(object);

        for (SceneObjectIterator<Object3D> itr = object->beginIterateChildren(); itr != object->endIterateChildren(); ++itr) {
            WeakPointer<Object3D> obj = *itr;
            this->processScene(obj, outObjects, nextTransform);
        }
    }

    void Renderer::renderReflectionProbe(WeakPointer<ReflectionProbe> reflectionProbe, Bool specularOnly,
                                         std::vector<WeakPointer<Object3D>>& renderObjects, std::vector<WeakPointer<Light>>& renderLights) {
        WeakPointer<Graphics> graphics = Engine::instance()->getGraphicsSystem();
        WeakPointer<Camera> probeCam = reflectionProbe->getRenderCamera();
        std::vector<WeakPointer<Object3D>> emptyObjectList;

        probeCam->setRenderTarget(reflectionProbe->getSceneRenderTarget());
        if (reflectionProbe->isSkyboxOnly()) {
            this->render(probeCam, emptyObjectList, renderLights, WeakPointer<Material>::nullPtr(), false);
        }
        else {
            this->render(probeCam, renderObjects, renderLights, WeakPointer<Material>::nullPtr(), false);
        }
        reflectionProbe->getSceneRenderTarget()->getColorTexture()->updateMipMaps();

        if(!specularOnly) {
            probeCam->setRenderTarget(reflectionProbe->getIrradianceMap());
            this->renderObjectBasic(reflectionProbe->getSkyboxObject(), probeCam, reflectionProbe->getIrradianceRendererMaterial());
        }
        
        WeakPointer<RenderTargetCube> specularIBLPreFilteredMap = reflectionProbe->getSpecularIBLPreFilteredMap();
        probeCam->setRenderTarget(specularIBLPreFilteredMap);
        WeakPointer<SpecularIBLPreFilteredRendererMaterial> specularIBLPreFilteredRendererMaterial = reflectionProbe->getSpecularIBLPreFilteredRendererMaterial();
        specularIBLPreFilteredRendererMaterial->setTextureResolution(specularIBLPreFilteredMap->getSize().x);
        for(UInt32 i = 0; i <= specularIBLPreFilteredMap->getMaxMipLevel(); i++) {
            specularIBLPreFilteredMap->setMipLevel(i);
            Real roughness = (Real)i / (Real)(specularIBLPreFilteredMap->getMaxMipLevel());
            specularIBLPreFilteredRendererMaterial->setRoughness(roughness);
            this->renderObjectBasic(reflectionProbe->getSkyboxObject(), probeCam, specularIBLPreFilteredRendererMaterial);
        }

        WeakPointer<RenderTarget2D> specularIBLBRDFMap = reflectionProbe->getSpecularIBLBRDFMap();
        graphics->renderFullScreenQuad(specularIBLBRDFMap, -1, reflectionProbe->getSpecularIBLBRDFRendererMaterial());
        
        reflectionProbe->setNeedsFullUpdate(false);
    }

    Bool Renderer::isShadowCastingCapableLight(WeakPointer<Light> light) {
        LightType lightType = light->getType();
        if (lightType == LightType::Ambient || lightType == LightType::Planar) {
            return false;
        }
        return true;
    }

    Bool Renderer::compareLights (WeakPointer<Light> a, WeakPointer<Light> b) { 
        return ((UInt32)a->getType() < (UInt32)b->getType()); 
    }

}
