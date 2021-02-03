/************************************************

	Assignment 1 - The Basics
	Maddin Thom - 100751351

	A lot of the code I used for this assignment 
	is from a tutorial from last semester, my
	GDW group engine and a terrain assignment
	from last semester

 ************************************************/

#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Game/Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Behaviours/CameraControlBehaviour.h"
#include "Behaviours/FollowPathBehaviour.h"
#include "Behaviours/SimpleMoveBehaviour.h"
#include "Game/Application.h"
#include "Game/GameObjectTag.h"
#include "Game/IBehaviour.h"
#include "Game/Transform.h"
#include "Graphics/Texture2D.h"
#include "Graphics/Texture2DData.h"
#include "Tools/InputHelpers.h"
#include "Tools/MeshBuilder.h"
#include "Tools/MeshFactory.h"
#include "Tools/NotObjLoader.h"
#include "Tools/ObjLoader.h"
#include "Tools/VertexTypes.h"
#include "Game/Scene.h"
#include "Game/ShaderMaterial.h"
#include "Game/RendererComponent.h"
#include "Game/Timing.h"
#include "Graphics/TextureCubeMap.h"
#include "Graphics/TextureCubeMapData.h"

#define LOG_GL_NOTIFICATIONS

 /*
	 Handles debug messages from OpenGL
	 https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	 @param source    Which part of OpenGL dispatched the message
	 @param type      The type of message (ex: error, performance issues, deprecated behavior)
	 @param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	 @param severity  The severity of the message (from High to Notification)
	 @param length    The length of the message
	 @param message   The human readable message from OpenGL
	 @param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
 */

// debugging
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
#endif
	default: break;
	}
}

// window
GLFWwindow* window;
void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	Application::Instance().ActiveScene->Registry().view<Camera>().each([=](Camera& cam) {
		cam.ResizeWindow(width, height);
		});
}

// initialize GLFW
bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	//Create a new GLFW window
	window = glfwCreateWindow(1000, 1000, "Assignment 1 - The Basics", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	// Store the window in the application singleton
	Application::Instance().Window = window;

	return true;
}

// initialize GLAD
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

// initialize ImGui menu
void InitImGui() {
	// Creates a new ImGUI context
	ImGui::CreateContext();
	// Gets our ImGUI input/output 
	ImGuiIO& io = ImGui::GetIO();
	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// Allow docking to our window
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// Allow multiple viewports (so we can drag ImGui off our window)
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// Allow our viewports to use transparent backbuffers
	io.ConfigFlags |= ImGuiConfigFlags_TransparentBackbuffers;

	// Set up the ImGui implementation for OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");

	// Dark mode FTW
	ImGui::StyleColorsDark();

	// Get our imgui style
	ImGuiStyle& style = ImGui::GetStyle();
	//style.Alpha = 1.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	}
}

// Shutdown ImGui menu
void ShutdownImGui()
{
	// Cleanup the ImGui implementation
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	// Destroy our ImGui context
	ImGui::DestroyContext();
}

// Render the ImGui menu
std::vector<std::function<void()>> imGuiCallbacks;
void RenderImGui() {
	// Implementation new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	// ImGui context new frame
	ImGui::NewFrame();

	if (ImGui::Begin("Debug")) {
		// Render our GUI stuff
		for (auto& func : imGuiCallbacks) {
			func();
		}
		ImGui::End();
	}

	// Make sure ImGui knows how big our window is
	ImGuiIO& io = ImGui::GetIO();
	int width{ 0 }, height{ 0 };
	glfwGetWindowSize(window, &width, &height);
	io.DisplaySize = ImVec2((float)width, (float)height);

	// Render all of our ImGui elements
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// If we have multiple viewports enabled (can drag into a new window)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		// Update the windows that ImGui is using
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		// Restore our gl context
		glfwMakeContextCurrent(window);
	}
}

// Render a vao
void RenderVAO(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const glm::mat4& viewProjection,
	const Transform& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", viewProjection * transform.WorldTransform());
	shader->SetUniformMatrix("u_Model", transform.WorldTransform());
	shader->SetUniformMatrix("u_NormalMatrix", transform.WorldNormalMatrix());
	vao->Render();
}

// Set up shader
void SetupShaderForFrame(const Shader::sptr& shader, const glm::mat4& view, const glm::mat4& projection) {
	shader->Bind();
	// These are the uniforms that update only once per frame
	shader->SetUniformMatrix("u_View", view);
	shader->SetUniformMatrix("u_ViewProjection", projection * view);
	shader->SetUniformMatrix("u_SkyboxMatrix", projection * glm::mat4(glm::mat3(view)));
	glm::vec3 camPos = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
	shader->SetUniform("u_CamPos", camPos);
}

// main
int main() {
	Logger::Init();
	// Initialize GLFW
	if (!initGLFW())
		return 1;
	//Initialize GLAD
	if (!initGLAD())
		return 1;

	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select first object by default
	std::vector<GameObject> controllables; // controllable objects

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Scope to free memory later
	{
#pragma region Shaders and ImGui Menu

		// Load shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(-5.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 0.5f;
		float     lightSpecularPow = 5.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 1.0f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		Shader::sptr shader2 = Shader::Create(); // water's shader
		shader2->LoadShaderPartFromFile("shaders/vertex_water_shader.glsl", GL_VERTEX_SHADER);
		shader2->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader2->Link();

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader2->SetUniform("u_LightPos", lightPos);
		shader2->SetUniform("u_LightCol", lightCol);
		shader2->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader2->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader2->SetUniform("u_AmbientCol", ambientCol);
		shader2->SetUniform("u_AmbientStrength", ambientPow);
		shader2->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader2->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader2->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		shader2->SetUniform("waveOn", true);

		Texture2D::sptr waterDif = Texture2D::LoadFromFile("textures/water.jpg"); // water
		Texture2D::sptr specular = Texture2D::LoadFromFile("textures/specular.png"); // specular

		ShaderMaterial::sptr material_2 = ShaderMaterial::Create(); // water
		material_2->Shader = shader2;
		material_2->Set("s_Diffuse", waterDif);
		material_2->Set("s_Specular", specular);

		// We'll add some ImGui controls to control our shader
		imGuiCallbacks.push_back([&]() {
			if (ImGui::Button("Mode 1: Nothing"))
			{
				shader->SetUniform("u_AmbientLightStrength", 0.0f);
				shader->SetUniform("u_SpecularLightStrength", 0.0f);
				shader->SetUniform("u_AmbientStrength", 0.0f);
				shader2->SetUniform("u_AmbientLightStrength", 0.0f);
				shader2->SetUniform("u_SpecularLightStrength", 0.0f);
				shader2->SetUniform("u_AmbientStrength", 0.0f);
				material_2->Set("waveOn", false);
			}
			if (ImGui::Button("Mode 2: Ambient"))
			{
				shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader->SetUniform("u_SpecularLightStrength", 0.0f);
				shader->SetUniform("u_AmbientStrength", ambientPow);
				shader2->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader2->SetUniform("u_SpecularLightStrength", 0.0f);
				shader2->SetUniform("u_AmbientStrength", ambientPow);
				material_2->Set("waveOn", false);
			}
			if (ImGui::Button("Mode 3: Specular"))
			{
				shader->SetUniform("u_AmbientLightStrength", 0.0f);
				shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader->SetUniform("u_AmbientStrength", 0.0f);
				shader2->SetUniform("u_AmbientLightStrength", 0.0f);
				shader2->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader2->SetUniform("u_AmbientStrength", 0.0f);
				material_2->Set("waveOn", false);
			}
			if (ImGui::Button("Mode 4: Ambient and Specular"))
			{
				shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader->SetUniform("u_AmbientStrength", ambientPow);
				shader2->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader2->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader2->SetUniform("u_AmbientStrength", ambientPow);
				material_2->Set("waveOn", false);
			}
			if (ImGui::Button("Mode 5: Everything"))
			{
				shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader->SetUniform("u_AmbientStrength", ambientPow);
				shader2->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				shader2->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				shader2->SetUniform("u_AmbientStrength", ambientPow);
				material_2->Set("waveOn", true);
			}
		});

		#pragma endregion

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);

		// Load textures

		Texture2D::sptr grassDif = Texture2D::LoadFromFile("textures/grass.jpg"); // grass
		Texture2D::sptr car1Dif = Texture2D::LoadFromFile("textures/car.png"); // car 1
		Texture2D::sptr car2Dif = Texture2D::LoadFromFile("textures/car2.png"); // car 2
		Texture2D::sptr tunnelDif = Texture2D::LoadFromFile("textures/tunnel.jpg"); // tunnel
		Texture2D::sptr roadDif = Texture2D::LoadFromFile("textures/road.png"); // road

		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		GameScene::sptr scene = GameScene::Create("Assignment");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// material 1 for grass, material 2 for water
		// material 3 for car 1, material 4 for car 2
		// material 5 for tunnel, material 6 for road
		ShaderMaterial::sptr material_1 = ShaderMaterial::Create(); // grass
		material_1->Shader = shader;
		material_1->Set("s_Diffuse", grassDif);
		material_1->Set("s_Specular", specular);

		ShaderMaterial::sptr material_3 = ShaderMaterial::Create(); // car 1
		material_3->Shader = shader;
		material_3->Set("s_Diffuse", car1Dif);
		material_3->Set("s_Specular", specular);

		ShaderMaterial::sptr material_4 = ShaderMaterial::Create(); // car 2
		material_4->Shader = shader;
		material_4->Set("s_Diffuse", car2Dif);
		material_4->Set("s_Specular", specular);

		ShaderMaterial::sptr material_5 = ShaderMaterial::Create(); // tunnel
		material_5->Shader = shader;
		material_5->Set("s_Diffuse", tunnelDif);
		material_5->Set("s_Specular", specular);

		ShaderMaterial::sptr material_6 = ShaderMaterial::Create(); // road
		material_6->Shader = shader;
		material_6->Set("s_Diffuse", roadDif);
		material_6->Set("s_Specular", specular);

		// plane object for the height map effect
		GameObject plane = scene->CreateEntity("plane");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("objects/flat.obj");
			plane.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material_1);
			plane.get<Transform>().SetLocalPosition(-5.0f, 0.0f, 1.0f).SetLocalScale(10.0f, 10.0f, 10.0f);

		}
		// water object for the wave effect
		GameObject water = scene->CreateEntity("water");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("objects/flat.obj");
			water.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material_2);
			water.get<Transform>().SetLocalPosition(-5.0f, 7.5f, 1.05f).SetLocalScale(10.0f, 10.0f, 10.0f);
		}

		// tunnel objects
		VertexArrayObject::sptr tunnelVAO = ObjLoader::LoadFromFile("objects/tunnel3.obj");
		GameObject tunnel = scene->CreateEntity("tunnel");
		{
			tunnel.emplace<RendererComponent>().SetMesh(tunnelVAO).SetMaterial(material_5);
			tunnel.get<Transform>().SetLocalPosition(1.0f, 2.25f, 1.0f).SetLocalRotation(-90.0f, 180.0f, 90.0f).SetLocalScale(0.375f, 0.375f, 0.375f);
		}
		GameObject tunnel2 = scene->CreateEntity("tunnel2");
		{
			tunnel2.emplace<RendererComponent>().SetMesh(tunnelVAO).SetMaterial(material_5);
			tunnel2.get<Transform>().SetLocalPosition(-15.0f, 2.25f, 1.0f).SetLocalRotation(-90.0f, 180.0f, 90.0f).SetLocalScale(0.375f, 0.375f, 0.375f);
		}

		// road object
		GameObject road = scene->CreateEntity("road");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("objects/road.obj");
			road.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material_6);
			road.get<Transform>().SetLocalPosition(-12.0f, -2.0f, 1.5f).SetLocalRotation(90.0f, 0.0f, 90.0f).SetLocalScale(0.3f, 0.3f, 0.3f);
		}

		// car objects
		VertexArrayObject::sptr carVAO = ObjLoader::LoadFromFile("objects/car.obj");
		GameObject car = scene->CreateEntity("car"); // car object
		{
			car.emplace<RendererComponent>().SetMesh(carVAO).SetMaterial(material_3);
			car.get<Transform>().SetLocalPosition(2.0f, 0.75f, 1.5f).SetLocalRotation(90.0f, 0.0f, -90.0f).SetLocalScale(0.5f, 0.5f, 0.5f);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(car);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -12.75f, 0.75f, 1.5f });
			pathing->Points.push_back({ 2.0f, 0.75f, 1.5f });
			pathing->Speed = 3.5f;
		}
		GameObject car2 = scene->CreateEntity("car2"); // car object 2
		{
			car2.emplace<RendererComponent>().SetMesh(carVAO).SetMaterial(material_4);
			car2.get<Transform>().SetLocalPosition(2.0f, -0.65f, 1.5f).SetLocalRotation(90.0f, 0.0f, -90.0f).SetLocalScale(0.5f, 0.5f, 0.5f);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing2 = BehaviourBinding::Bind<FollowPathBehaviour>(car2);
			// Set up a path for the object to follow
			pathing2->Points.push_back({ -12.75f, -0.65f, 1.5f });
			pathing2->Points.push_back({ 2.0f, -0.65f, 1.5f });
			pathing2->Speed = 3.0f;
		}

		// Create an object to be our camera
		GameObject cameraObj = scene->CreateEntity("Camera");
		{
			cameraObj.get<Transform>().SetLocalPosition(-5, 11, 7).LookAt(glm::vec3(-5, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObj.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 0, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObj);
		}

		InitImGui();

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		float waterTime = 0.0f; // float for water movement

		material_2->Set("time", waterTime);
		material_2->Set("waveOn", true);

		// GAME LOOP
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			material_2->Set("time", waterTime);
			waterTime += 0.1f;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			float carX = car.get<Transform>().GetLocalPosition().x;
			if (carX <= -12.5) {
				car.get<Transform>().SetLocalPosition(2.0f, 0.75f, 1.5f);
			}

			float car2X = car2.get<Transform>().GetLocalPosition().x;
			if (car2X <= -12.5) {
				car2.get<Transform>().SetLocalPosition(2.0f, -0.65f, 1.5f);
			}

			// Clear the screen
			glClearColor(0.18f, 0.37f, 0.61f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObj.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObj.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
				});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			// Draw our ImGui content
			RenderImGui();

			scene->Poll();
			glfwSwapBuffers(window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		ShutdownImGui();

	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0; 

} 
