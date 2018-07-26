#include "MeshRenderer.h"
#include "../Engine.h"
#include "../geometry/AttributeArray.h"
#include "../geometry/AttributeArrayGPUStorage.h"
#include "../geometry/Mesh.h"
#include "../image/Texture.h"
#include "../light/PointLight.h"
#include "../light/DirectionalLight.h"
#include "../material/Material.h"
#include "../material/Shader.h"
#include "../render/Camera.h"
#include "../render/RenderTarget.h"
#include "RenderableContainer.h"

namespace Core {

    MeshRenderer::MeshRenderer(WeakPointer<Graphics> graphics, WeakPointer<Material> material, WeakPointer<Object3D> owner)
        : ObjectRenderer<Mesh>(graphics, owner), material(material) {
    }

    Bool MeshRenderer::forwardRenderObject(const ViewDescriptor& viewDescriptor, WeakPointer<Mesh> mesh, const std::vector<WeakPointer<Light>>& lights) {
        WeakPointer<Material> material;
        if (viewDescriptor.overrideMaterial.isValid()) {
            material = viewDescriptor.overrideMaterial;
        } else {
            material = this->material;
        }

        WeakPointer<Shader> shader = material->getShader();
        this->graphics->activateShader(shader);

        // send custom uniforms first so that the renderer can override if necessary.
        material->sendCustomUniformsToShader();

        this->checkAndSetShaderAttribute(mesh, material, StandardAttribute::Position, mesh->getVertexPositions());
        this->checkAndSetShaderAttribute(mesh, material, StandardAttribute::Normal, mesh->getVertexNormals());
        this->checkAndSetShaderAttribute(mesh, material, StandardAttribute::FaceNormal, mesh->getVertexFaceNormals());
        this->checkAndSetShaderAttribute(mesh, material, StandardAttribute::Color, mesh->getVertexColors());
        this->checkAndSetShaderAttribute(mesh, material, StandardAttribute::UV0, mesh->getVertexUVs0());

        Int32 projectionLoc = material->getShaderLocation(StandardUniform::ProjectionMatrix);
        Int32 viewMatrixLoc = material->getShaderLocation(StandardUniform::ViewMatrix);
        Int32 modelMatrixLoc = material->getShaderLocation(StandardUniform::ModelMatrix);
        Int32 modelInverseTransposeMatrixLoc = material->getShaderLocation(StandardUniform::ModelInverseTransposeMatrix);
        Int32 viewInverseTransposeMatrixLoc = material->getShaderLocation(StandardUniform::ViewInverseTransposeMatrix);

        if (projectionLoc >= 0) {
            const Matrix4x4& projMatrix = viewDescriptor.projectionMatrix;
            shader->setUniformMatrix4(projectionLoc, projMatrix);
        }

        if (viewMatrixLoc >= 0) {
            ;
            Matrix4x4 viewMatrix = viewDescriptor.viewInverseMatrix;
            shader->setUniformMatrix4(viewMatrixLoc, viewMatrix);
        }

        if (modelMatrixLoc >= 0) {
            Matrix4x4 modelmatrix = this->owner->getTransform().getWorldMatrix();
            shader->setUniformMatrix4(modelMatrixLoc, modelmatrix);
        }

        if (modelInverseTransposeMatrixLoc >= 0) {
            Matrix4x4 modelInverseTransposeMatrix = this->owner->getTransform().getWorldMatrix();
            modelInverseTransposeMatrix.invert();
            modelInverseTransposeMatrix.transpose();
            shader->setUniformMatrix4(modelInverseTransposeMatrixLoc, modelInverseTransposeMatrix);
        }

        if (viewInverseTransposeMatrixLoc >= 0) {
            Matrix4x4 viewInverseTransposeMatrix = viewDescriptor.viewInverseTransposeMatrix;
            shader->setUniformMatrix4(viewInverseTransposeMatrixLoc, viewInverseTransposeMatrix);
        }

        Int32 lightRangeLoc = material->getShaderLocation(StandardUniform::LightRange);
        Int32 lightTypeLoc = material->getShaderLocation(StandardUniform::LightType);
        Int32 lightIntensityLoc = material->getShaderLocation(StandardUniform::LightIntensity);
        Int32 lightColorLoc = material->getShaderLocation(StandardUniform::LightColor);
        Int32 lightEnabledLoc = material->getShaderLocation(StandardUniform::LightEnabled);

        Int32 lightMatrixLoc = material->getShaderLocation(StandardUniform::LightMatrix);
        Int32 lightAngularShadowBiasLoc = material->getShaderLocation(StandardUniform::LightAngularShadowBias);
        Int32 lightConstantShadowBiasLoc = material->getShaderLocation(StandardUniform::LightConstantShadowBias);

        UInt32 currentTextureSlot = material->textureCount();

        if (lights.size() > 0) {
            if (lightEnabledLoc >= 0) {
                shader->setUniform1i(lightEnabledLoc, 1);
            }

            UInt32 renderedCount = 0;
            for (UInt32 i = 0; i < lights.size(); i++) {

                WeakPointer<Light> light = lights[i];
                LightType lightType = light->getType();

                if (renderedCount == 0) {
                    graphics->setBlendingEnabled(false);
                } else {
                    graphics->setBlendingEnabled(true);
                    graphics->setBlendingFunction(RenderState::BlendingMethod::One, RenderState::BlendingMethod::One);
                }

                if (lightColorLoc >= 0) {
                    Color color = light->getColor();
                    // std::cerr << " setting light color: " << color.r << ", " << color.g << ", " << color.b << std::endl;
                    shader->setUniform4f(lightColorLoc, color.r, color.g, color.b, color.a);
                }

                if (lightTypeLoc >= 0) {
                    shader->setUniform1i(lightTypeLoc, (Int32)lightType);
                }

                if (lightIntensityLoc >= 0) {
                    shader->setUniform1f(lightIntensityLoc, light->getIntensity());
                }

                if (lightMatrixLoc >= 0) {
                    shader->setUniformMatrix4(lightMatrixLoc, light->getOwner()->getTransform().getConstInverseWorldMatrix());
                }

                if (lightType == LightType::Point) {

                    WeakPointer<PointLight> pointLight = WeakPointer<Light>::dynamicPointerCast<PointLight>(light);

                    if (lightAngularShadowBiasLoc >= 0) {
                        shader->setUniform1f(lightAngularShadowBiasLoc, pointLight->getAngularShadowBias());
                    }

                    if (lightConstantShadowBiasLoc >= 0) {
                        shader->setUniform1f(lightConstantShadowBiasLoc, pointLight->getConstantShadowBias());
                    }
                    
                    if (lightRangeLoc >= 0) {
                        shader->setUniform1f(lightRangeLoc, pointLight->getRadius());
                    }

                    Int32 lightPositionLoc = material->getShaderLocation(StandardUniform::LightPosition);
                    if (lightPositionLoc >= 0) {
                        Point3r pos;
                        pointLight->getOwner()->getTransform().getWorldMatrix().transform(pos);
                        shader->setUniform4f(lightPositionLoc, pos.x, pos.y, pos.z, 1.0f);
                    }

                    Int32 lightShadowCubeMapLoc = material->getShaderLocation(StandardUniform::LightShadowCubeMap);
                    if (lightShadowCubeMapLoc >= 0 && pointLight->getShadowsEnabled()) {
                        shader->setTextureCube(currentTextureSlot, pointLight->getShadowMap()->getColorTexture()->getTextureID());
                        shader->setUniform1i(lightShadowCubeMapLoc, currentTextureSlot);
                        currentTextureSlot++;
                    }
                }
                else if (lightType == LightType::Directional) {
                    WeakPointer<DirectionalLight> directionalLight = WeakPointer<Light>::dynamicPointerCast<DirectionalLight>(light);

                    if (lightAngularShadowBiasLoc >= 0) {
                        shader->setUniform1f(lightAngularShadowBiasLoc, directionalLight->getAngularShadowBias());
                    }

                    if (lightConstantShadowBiasLoc >= 0) {
                        shader->setUniform1f(lightConstantShadowBiasLoc, directionalLight->getConstantShadowBias());
                    }

                    Int32 lightDirectionLoc = material->getShaderLocation(StandardUniform::LightDirection);
                    if (lightDirectionLoc >= 0) {
                        Vector3r dir = Vector3r::Forward;
                        directionalLight->getOwner()->getTransform().getWorldMatrix().transform(dir);
                        shader->setUniform4f(lightDirectionLoc, dir.x, dir.y, dir.z, 0.0f);
                    }

                    UInt32 cascadeCount = directionalLight->getCascadeCount();

                    Int32 cascadeCountLoc = material->getShaderLocation(StandardUniform::LightCascadeCount);
                    if (cascadeCountLoc >= 0) {
                        //std:: cerr << "setting cascade count: " << cascadeCountLoc << ", " << cascadeCount << std::endl;
                        shader->setUniform1i(cascadeCountLoc, cascadeCount);
                    }

                    for (UInt32 l = 0; l < cascadeCount; l++) {
                        Int32 shadowMapLoc = material->getShaderLocation(StandardUniform::LightShadowMap, l);
                        if (shadowMapLoc >= 0) {
                            shader->setTexture2D(currentTextureSlot, directionalLight->getShadowMap(l)->getColorTexture()->getTextureID());
                            shader->setUniform1i(shadowMapLoc, currentTextureSlot);
                            currentTextureSlot++;
                        }

                        Int32 viewProjectionLoc = material->getShaderLocation(StandardUniform::LightViewProjection, l);
                        if (viewProjectionLoc >= 0) {
                            shader->setUniformMatrix4(viewProjectionLoc, directionalLight->getProjectionMatrix(l));
                        }

                        Int32 cascadeEndLoc = material->getShaderLocation(StandardUniform::LightCascadeEnd, l);
                        if (cascadeEndLoc >= 0) {
                            shader->setUniform1f(cascadeEndLoc, directionalLight->getCascadeBoundary(l + 1));
                        }
                    }
                }
                renderedCount++;
                this->drawMesh(mesh);
            }

        } else {
            if (lightEnabledLoc >= 0) {
                shader->setUniform1i(lightEnabledLoc, 0);
            }
            this->drawMesh(mesh);
        }

        return true;
    }

    Bool MeshRenderer::forwardRender(const ViewDescriptor& viewDescriptor, const std::vector<WeakPointer<Light>>& lights) {
        std::shared_ptr<RenderableContainer<Mesh>> thisContainer = std::dynamic_pointer_cast<RenderableContainer<Mesh>>(this->owner.lock());
        if (thisContainer) {
            auto renderables = thisContainer->getRenderables();
            for (auto mesh : renderables) {
                this->forwardRenderObject(viewDescriptor, mesh, lights);
            }
        }

        return true;
    }

    Bool MeshRenderer::supportsRenderPath(RenderPath renderPath) {
        if (renderPath == RenderPath::Forward) return true;
        return false;
    }

    void MeshRenderer::checkAndSetShaderAttribute(WeakPointer<Mesh> mesh, WeakPointer<Material> material, StandardAttribute attribute,
                                                  WeakPointer<AttributeArrayBase> array) {
        if (mesh->isAttributeEnabled(attribute)) {
            Int32 shaderLocation = material->getShaderLocation(attribute);
            if (array->getGPUStorage()) {
                array->getGPUStorage()->sendToShader(shaderLocation);
            }
        }
    }

    void MeshRenderer::drawMesh(WeakPointer<Mesh> mesh) {
        if (mesh->isIndexed()) {
            this->graphics->drawBoundVertexBuffer(mesh->getIndexCount(), mesh->getIndexBuffer());
        } else {
            this->graphics->drawBoundVertexBuffer(mesh->getVertexCount());
        }
    }
}