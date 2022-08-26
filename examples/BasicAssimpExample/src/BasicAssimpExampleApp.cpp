#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Timeline.h"
#include "cinder/Log.h"
#include "cinder/CinderImGui.h"
#include "AssimpLoader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicAssimpExampleApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseWheel(MouseEvent event) override;
    void keyDown(KeyEvent event) override;
	std::vector<sitara::assimp::AssimpLoader> mAssimpModels;
    std::vector<std::string> mAssimpModelNames;
    std::vector<std::string> mMeshNames;
    std::deque<bool> mShowMeshes;

	ci::CameraUi mCameraUi;
	ci::CameraPersp mCamera;
	ci::Anim<ci::vec2> mMousePos;
    ci::vec3 mModelAngles;
    ci::vec3 mCameraAngles;
    ci::gl::GlslProgRef mCustomShader;
    ci::ColorA mTintColor;
    float mTintRatio;

	bool mEnableTextures;
	bool mEnableWireframe;
    bool mEnableMaterials;
	bool mEnableSkinning;
	bool mEnableAnimation;
    bool mEnableCustomShader;
	bool mDrawBBox;
    bool mDrawBones;
	float mFps;
    int mModelSelect = 0;
    int mAnimationSelect = 0;
};

void BasicAssimpExampleApp::setup() {
    ci::app::addAssetDirectory("../../../assets/");

    CI_LOG_I("Loading 3d models: ");
 //   auto teapot = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/teapot.obj"));
 //   mAssimpModels.push_back(teapot);
 //   mAssimpModelNames.push_back("Plain Utah Teapot");

 //   auto teapot_red = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/teapot_red.obj"));
 //   mAssimpModels.push_back(teapot_red);
 //   mAssimpModelNames.push_back("Red Utah Teapot (.obj)");

	//auto teapot_ply = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/teapot_red.ply"));
 //   mAssimpModels.push_back(teapot_ply);
 //   mAssimpModelNames.push_back("Red Utah Teapot (.ply)");

	//auto teapot_tex = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/teapot_textured.obj"));
 //   mAssimpModels.push_back(teapot_tex);
 //   mAssimpModelNames.push_back("Textured Utah Teapot");

	//auto spot = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/spot.obj"));
 //   mAssimpModels.push_back(spot);
 //   mAssimpModelNames.push_back("Spot");

 //   auto benchy_stl = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/3DBenchy.stl"));
 //   mAssimpModels.push_back(benchy_stl);
 //   mAssimpModelNames.push_back("3DBenchy (.stl)");

 //   auto benchy_3mf = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/3DBenchy.3mf"));
 //   mAssimpModels.push_back(benchy_3mf);
 //   mAssimpModelNames.push_back("3DBenchy (.3mf)");

    auto sloth = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/Harlans-ground-sloth.fbx"));
    sloth.setAnimation(1);
    mAssimpModels.push_back(sloth);
    mAssimpModelNames.push_back("Harlan's Ground Sloth (.fbx)");

    auto mastodon = sitara::assimp::AssimpLoader(ci::app::getAssetPath("models/Mastodon.fbx"));
    mastodon.setAnimation(1);
    mAssimpModels.push_back(mastodon);
    mAssimpModelNames.push_back("Mastodon (.fbx)");

    mEnableWireframe = false;
    mEnableTextures = true;
    mEnableMaterials = false;
    mEnableSkinning = false;
    mEnableAnimation = false;
    mEnableCustomShader = false;
    mDrawBBox = false;
    mDrawBones = false;

	mCamera.setPerspective(60, getWindowAspectRatio(), 1.0f, 20000.0f);
    mCamera.lookAt(ci::vec3(-20, 2, -4), ci::vec3(0), ci::vec3(0, 1, 0));
    mCameraUi = ci::CameraUi(&mCamera);

    mCameraAngles = ci::vec3(0.0);
    mModelAngles = ci::vec3(0.0);
    
    auto vertexShader = ci::app::loadAsset(ci::app::getAssetPath("glsl/vertex/tint.vert"));
    auto fragmentShader = ci::app::loadAsset(ci::app::getAssetPath("glsl/frag/lambert.frag"));

    mCustomShader = ci::gl::GlslProg::create(vertexShader,
                                             fragmentShader);

    mTintColor = ci::ColorA(0, 1, 0, 1);
    mTintRatio = 0.5f;

	ImGui::Initialize();
}

void BasicAssimpExampleApp::update()
{
    ImGui::Begin("Assimp Test Menu");
    bool isChanged;
    if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ListBoxHeader("Preloaded Models");
        int counter = 0;
        for (auto& item : mAssimpModelNames) {
            if (ImGui::Selectable(item.c_str(), counter == mModelSelect)) {
                mModelSelect = counter;
            }
            mMeshNames.clear();
            mShowMeshes.clear();
            for (int i = 0; i < mAssimpModels[mModelSelect].getNumMeshes(); i++) {
                auto mesh = mAssimpModels[mModelSelect].getMesh(i);
                mMeshNames.push_back(mesh->mName);
                mShowMeshes.push_back(mesh->mShowMesh);
            }
            counter++;
        }
        ImGui::ListBoxFooter();
    }
    if (ImGui::CollapsingHeader("Mesh Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < mMeshNames.size(); i++) {
            isChanged = ImGui::Checkbox(mMeshNames[i].c_str(), &mShowMeshes[i]);
            if (isChanged) {
                mAssimpModels[mModelSelect].getMesh(i)->mShowMesh = mShowMeshes[i];
            }
        }
        ImGui::ListBoxHeader("Animation Selection");
        int counter = 0;
        for (auto& item : mAssimpModels[mModelSelect].getAnimationNames()) {
            if (ImGui::Selectable(item.c_str(), counter == mAnimationSelect)) {
                mAnimationSelect = counter;
                mAssimpModels[mModelSelect].setAnimation(mAnimationSelect);
            }
            counter++;
        }
        ImGui::ListBoxFooter();

    }
    if (ImGui::CollapsingHeader("Render Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        isChanged = ImGui::Checkbox("Use Wireframe", &mEnableWireframe);
        isChanged = ImGui::Checkbox("Use Textures", &mEnableTextures);
        if (isChanged) {
            for (auto& model : mAssimpModels) {
                model.enableTextures(mEnableTextures);
            }
        }
        isChanged = ImGui::Checkbox("Use Materials", &mEnableMaterials);
        if (isChanged) {
            for (auto& model : mAssimpModels) {
                model.enableMaterials(mEnableMaterials);
            }
        }
        isChanged = ImGui::Checkbox("Use Skinning", &mEnableSkinning);
        if (isChanged) {
            for (auto& model : mAssimpModels) {
                model.enableSkinning(mEnableSkinning);
            }
        }
        isChanged = ImGui::Checkbox("Use Animation", &mEnableAnimation);
        if (isChanged) {
            for (auto& model : mAssimpModels) {
                model.enableAnimation(mEnableAnimation);
            }
        }
        isChanged = ImGui::Checkbox("Use Custom Shader", &mEnableCustomShader);
        if (isChanged) {
            for (auto& model : mAssimpModels) {
                model.enableCustomShader(mEnableCustomShader);
                model.setCustomShader(mCustomShader);
            }
        }
        isChanged = ImGui::Checkbox("Draw BBox", &mDrawBBox);
        isChanged = ImGui::Checkbox("Draw Bones", &mDrawBones);
    }

    if (ImGui::CollapsingHeader("Camera Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ci::vec3 eyePoint = mCamera.getEyePoint();
        ci::vec3 up = mCamera.getWorldUp();
        mCameraAngles = glm::eulerAngles(mCamera.getOrientation());

        isChanged = ImGui::SliderFloat3("Eye Position", (float*)&eyePoint, -100, 100);
        if (isChanged) {
            mCamera.setEyePoint(eyePoint);
        }
        isChanged = ImGui::SliderFloat3("Camera Rotation", (float*)&mCameraAngles, -2.0f * M_PI, 2.0f * M_PI);
        if (isChanged) {
            mCamera.setOrientation(ci::quat(mCameraAngles));
        }
        isChanged = ImGui::SliderFloat3("Object Rotation", (float*)&mModelAngles, -2.0f * M_PI, 2.0f * M_PI);
        isChanged = ImGui::Button("Reset Camera");
        if (isChanged) {
            mCamera.lookAt(vec3(0.0, 7.0, 20.0), vec3(0.0, 0.0, 0.0), vec3(0, 1, 0));
        }
    }
    if (ImGui::CollapsingHeader("Custom Shader Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorPicker4("Tint Color", &mTintColor);
        ImGui::SliderFloat("Tint Ratio", &mTintRatio, 0.0f, 1.0f);
    }
    ImGui::End();

    if (mEnableAnimation && mAssimpModels[mModelSelect].getNumAnimations()) {
        double time = fmod(getElapsedSeconds(), mAssimpModels[mModelSelect].getAnimationDuration(1));
        mAssimpModels[mModelSelect].setTime(time);
        mAssimpModels[mModelSelect].update();
    }

	mFps = getAverageFps();
}

void BasicAssimpExampleApp::draw() {
	ci::gl::clear(Color::white());

	ci::gl::setMatrices(mCamera);

	ci::gl::enableDepthWrite();
	ci::gl::enableDepthRead();

	ci::gl::color(Color::white());

	if (mEnableWireframe) {
		ci::gl::enableWireframe();
	}

    if (mEnableCustomShader) {
        mCustomShader->uniform("tintColor", mTintColor);
        mCustomShader->uniform("tintRatio", mTintRatio);
    }

	mAssimpModels[mModelSelect].draw();

    if (mEnableWireframe) {
		ci::gl::disableWireframe();
	}

	if (mDrawBBox) {
        ci::gl::ScopedColor black(ci::Color::black());
		ci::gl::drawStrokedCube(mAssimpModels[mModelSelect].getBoundingBox());
	}

    if (mDrawBones) {
            //mAssimpModels[mModelSelect].get
    }

	ci::gl::disableDepthRead();
	ci::gl::disableDepthWrite();
}

void BasicAssimpExampleApp::mouseDown(MouseEvent event) {
	mMousePos.stop();
	mMousePos = event.getPos();
	mCameraUi.mouseDown(mMousePos);
}

void BasicAssimpExampleApp::mouseDrag(MouseEvent event) {
	timeline().apply(&mMousePos, vec2(event.getPos()), 1.0f, EaseOutQuint()).updateFn([=]
		{
			mCameraUi.mouseDrag(mMousePos(), event.isLeftDown(), false, event.isRightDown());
		});
}

void BasicAssimpExampleApp::mouseWheel(MouseEvent event) {
	mCameraUi.mouseWheel(event.getWheelIncrement());
}

void BasicAssimpExampleApp::keyDown(KeyEvent event) {
    if (event.getCode() == KeyEvent::KEY_UP) {
        ci::vec3 right, up;
        mCamera.getBillboardVectors(&right, &up);
        right *= 0;
        up *= 10;
        mCamera.setEyePoint(mCamera.getEyePoint() - right + up);    
    } else if (event.getCode() == KeyEvent::KEY_DOWN) {
        ci::vec3 right, up;
        mCamera.getBillboardVectors(&right, &up);
        right *= 0;
        up *= -10;
        mCamera.setEyePoint(mCamera.getEyePoint() - right + up);    
    } else if (event.getCode() == KeyEvent::KEY_RIGHT) {
        ci::vec3 right, up;
        mCamera.getBillboardVectors(&right, &up);
        right *= 10;
        up *= 0;
        mCamera.setEyePoint(mCamera.getEyePoint() - right + up);    
    } else if (event.getCode() == KeyEvent::KEY_LEFT) {
        ci::vec3 right, up;
        mCamera.getBillboardVectors(&right, &up);
        right *= -10;
        up *= 0;
        mCamera.setEyePoint(mCamera.getEyePoint() - right + up);
    }
}

CINDER_APP(BasicAssimpExampleApp, RendererGl, [=](cinder::app::App::Settings* settings) {
	settings->setConsoleWindowEnabled();
	settings->setWindowSize(1920, 1080);
	})