
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;
 

int main()
{

    Device &device = Device::Instance();
    
    if (!device.Create(screenWidth, screenHeight, "Game",true))
    {
        return 1;
    }
    
    RenderBatch batch;
    batch.Init();
    batch.SetColor(255, 255, 255);
    Mat4  ortho = Mat4::Ortho(0, device.GetWidth(), device.GetHeight(),  0, -1, 1);
    batch.SetMatrix(ortho);

    RenderBatch batch3D;
    batch3D.Init();

    Font font;
    font.LoadDefaultFont();
    font.SetFontSize(16);
    font.SetBatch(&batch);


    Shader* shader = ShaderManager::Instance().Get("3DShader");
    Shader* solid = ShaderManager::Instance().Get("Default");
    Texture2D* diffuse =  TextureManager::Instance().Load("Rocks_Diffuse.png", "diffuse");
    Texture2D* normal  =  TextureManager::Instance().Load("Rocks_Normal.png", "normal");

    Texture2D* cubeDiffuse =  TextureManager::Instance().Load("rockwall.tga", "cube_diffuse");
    Texture2D* cubeNormal  =  TextureManager::Instance().Load("rockwall_NH.tga", "cube_normal");

    diffuse->SetAnisotropicFiltering(8.0f);
    
    

    Camera camera;

    Mat4 projection = camera.GetProjectionMatrix((float)screenWidth / (float)screenHeight);
    Mat4 mvp = projection * camera.GetViewMatrix();
    batch3D.SetMatrix(mvp);

    Scene& scene = Scene::Instance();
    


    Mesh* cube = MeshManager::Instance().CreateCube(1.0f);
    Mesh* plane = MeshManager::Instance().CreatePlane(10.0f, 10.0f,16,16, 5, 5);
    Mesh* sphere = MeshManager::Instance().CreateSphere(0.5f, 16, 16);
    Mesh* cylinder = MeshManager::Instance().CreateCylinder(1.0f, 2.0f, 16);

    Model* floor = scene.CreateModel();
    floor->AddMesh(plane);
    floor->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);
    

    Model* sphereModel = scene.CreateModel();
    sphereModel->AddMesh(sphere);
    sphereModel->SetPosition(Vec3(0.0f, 1.0f, 2.0f));
    sphereModel->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);

    Model* cylinderModel = scene.CreateModel();
    cylinderModel->AddMesh(cylinder);
    cylinderModel->SetPosition(Vec3(2.0f, 1.0f, 0.0f));
    cylinderModel->SetScale(Vec3(0.5f, 0.5f, 0.5f));
    cylinderModel->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);


    Model* cubeModel = scene.CreateModel();
    cubeModel->AddMesh(cube);
    cubeModel->SetPosition(Vec3(-2.0f, 1.0f, -2.0f));
    cubeModel->SetScale(Vec3(0.5f, 0.5f, 0.5f));
    cubeModel->AddMaterial()->SetTexture(0, cubeDiffuse)->SetTexture(1, cubeNormal);
    


    float bump = 0.8f;

    while (device.Run())
    {

        const float dt = device.GetFrameTime();
        float SPEED = 0.1f * dt;
        
        if (device.IsResize())
        {
            glViewport(0, 0, device.GetWidth(), device.GetHeight());
            
            Mat4  ortho = Mat4::Ortho(0, device.GetWidth(), device.GetHeight(),  0, -1, 1);
            batch.SetMatrix(ortho);
            
            projection = camera.GetProjectionMatrix((float)screenWidth / (float)screenHeight);
            
            
        }
        Mat4 view = camera.GetViewMatrix();
        mvp = projection * view;
        batch3D.SetMatrix(mvp);
        
        camera.MouseView(dt);
        if (Keyboard::Down(KEY_W))
        {
            camera.ProcessKeyboard(0, SPEED);
        } 
        if (Keyboard::Down(KEY_S))
        {
            camera.ProcessKeyboard(1, SPEED);
        }
        if (Keyboard::Down(KEY_A))
        {
            camera.ProcessKeyboard(2, SPEED);
        }
        if (Keyboard::Down(KEY_D))
        {
            camera.ProcessKeyboard(3, SPEED);
        }


        if (Keyboard::Down(KEY_UP))
        {
            bump += 0.01f;
        }
        if (Keyboard::Down(KEY_DOWN))
        {
            bump -= 0.01f;
        }
        
        if (bump < 0.0f)
        {
            bump = 0.0f;
        }
        if (bump > 2.0f)
        {
            bump = 2.0f;
        }
        

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cubeModel->Rotate(Vec3(0.0f, 1.0f, 1.0f));


        Mat4 model = Mat4::Identity();

        shader->Use();
        shader->SetMatrix4("model", model.x);
        shader->SetMatrix4("view", view.x);
        shader->SetMatrix4("proj", projection.x);
        shader->SetFloat("bumpScale", bump);

        
        scene.Update(dt);
        scene.SetShader(shader);
        scene.Render();

        shader->Use(false);

        model = Mat4::Translate(0.0f, 1.0f, 0.0f);


        solid->Use();
        solid->SetMatrix4("model", model.x);
        solid->SetMatrix4("view", view.x);
        solid->SetMatrix4("proj", projection.x);
        

        solid->Use(false);


        


        batch3D.SetColor(1.0f, 1.0f, 1.0f);
 
        batch3D.Grid(10, 10);
        batch3D.Render();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);//good 4 text

        batch.SetColor(1.0f, 1.0f, 1.0f);
        batch.Line2D(Vec2(0, 0), Vec2(100, 100));
        
        batch.SetColor(0.0f, 1.0f, 0.0f);
        font.Print(10,10,"FPS: %d", (int)(device.GetFPS()));
        font.Print(10,20,"Bump: %f", bump);
        batch.Render();
        
        device.Flip();
    }

    scene.Clear();
    font.Release();
    batch3D.Release();
    batch.Release();
    device.Close();



    return 0;
}
