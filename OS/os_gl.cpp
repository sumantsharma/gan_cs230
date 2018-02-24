// OS_GL.HPP
// ------------------------------------------------------------------------
// DESCRIPTION: abstraction of openGL with interfaces for CAD models
// ------------------------------------------------------------------------
// AUTHOR: Connor Beierle
//         2018-01-30 CRB: major overhaul
// ------------------------------------------------------------------------
// COPYRIGHT: 2016 SLAB Group
//            OS Function
// ------------------------------------------------------------------------

#include "os_gl.hpp"
//#include "mex.h"

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb/stb_image_write.h"

void CallbackFrameBufferSize(GLFWwindow* window, int width, int height);
void CallbackMouse(GLFWwindow* window, double xpos, double ypos);
void CallbackScroll(GLFWwindow* window, double xoffset, double yoffset);
void CallbackWindowClose(GLFWwindow* window);
static void CallbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);

float lastX = 1;
float lastY = 1;
bool firstMouse = true;
bool mouse_input = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// global buffers
const int BUFFER_SIZE_VERTEX = 100000;
const int BUFFER_SIZE_PARTS = 1000;

GL::GL()
{
    m_camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
}

GL::GL(int Nu, int Nv, double ppx, double ppy, double fx, double fy){
    mouse_input = false;
    
    // camera properties
    m_camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    m_camera.dx = ppx;
    m_camera.dy = ppy;
    m_camera.fx = fx;
    m_camera.fy = fy;
    m_camera.Nu = Nu;
    m_camera.Nv = Nv;
    m_camera.FOV_vertical_deg = 2*atan(m_camera.Nv*m_camera.dy/m_camera.fy/2) * RAD2DEG;

    Create_Window();

};

GL::~GL()
{
    DeleteCAD(m_star);
    DeleteCAD(m_tango);
    DeleteCAD(m_earth);
    DeleteCAD(m_sun);
    DeleteCAD(m_moon);
    DeleteCAD(m_lamp);
    DeleteCAD(m_triad);
    DeleteCAD(m_cube);

    glfwTerminate();
}

void GL::Terminate(){
    glfwTerminate();
}

int GL::Create_Window(){
    // kill any old windows
    glfwTerminate();
    
    // start fresh
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // add decorator (the bar on top of applications)
    if (m_decoratorOn)
        glfwWindowHint(GLFW_DECORATED, true);
    else
        glfwWindowHint(GLFW_DECORATED, false);
        
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // OS X only
    #endif

    // glfw window creation
    //window = glfwCreateWindow(width_pix, height_pix, "OpticalStimulator", glfwGetPrimaryMonitor(), NULL);
    m_window = glfwCreateWindow(m_camera.Nu, m_camera.Nv, m_windowTitle, NULL, NULL);

    if (m_window == NULL){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(m_window);
    //glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetFramebufferSizeCallback(m_window, CallbackFrameBufferSize );
    glfwSetCursorPosCallback(m_window, CallbackMouse);
    glfwSetScrollCallback(m_window, CallbackScroll);
    glfwSetKeyCallback(m_window, CallbackKey);
    glfwSetWindowCloseCallback(m_window, CallbackWindowClose);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetWindowPos(m_window, m_windowPosX, m_windowPosY);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    
    glfwShowWindow(m_window);
    glfwSwapBuffers(m_window);

    return 0;
}

void GL::MaximizeWindow(){
    glfwMaximizeWindow(m_window);
}

void GL::SetDecorator(bool decoratorOn){
    m_decoratorOn = decoratorOn;
    Create_Window();
}

void GL::SetWindowPosition(int xPos, int yPos){
    m_windowPosX = xPos;
    m_windowPosY = yPos;
    glfwSetWindowPos(m_window, m_windowPosX, m_windowPosY);
}

void GL::GetWindowSize(int* outWidth, int* outHeight){
    glfwGetWindowSize(m_window, outWidth, outHeight);
}

void GL::GetMonitorSize(int* widthPIX, int* heightPIX){
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    *widthPIX = mode->width;
    *heightPIX = mode->height;
}

void GL::GetMonitorPhysicalSize(int* widthMM, int* heightMM){
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorPhysicalSize(monitor, widthMM, heightMM);
}

void GL::ClearScreen(){
    // clear the buffer array to prepare a new screen
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GL::SwapBuffers(){
    // clear the buffer array to prepare a new screen
    glfwSwapBuffers(m_window);
}

void GL::Screenshot(std::string filename) {

    // read pixels
    char *pixel_data = new char[3*m_camera.Nu*m_camera.Nv];
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, m_camera.Nu, m_camera.Nv, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);
    
    // detect file type
    std::string imageType = filename.substr(filename.length()-3, filename.length());
    std::transform(imageType.begin(), imageType.end(), imageType.begin(), ::tolower);
    
    // write to file
    int status;
    int stride_bytes = 3*m_camera.Nu*sizeof(pixel_data[0]);

    switch( m_mapImageType[imageType] )
    {
        case GL::IMAGE_TYPES::INVALID :
        {
            std::cout << "Unsupported image type for screenshot. Supported types are {png, jpg, bmp, tga}..." << std::endl;
            break;
        }
        case GL::IMAGE_TYPES::PNG :
        {
            status = stbi_write_png(filename.c_str(), m_camera.Nu, m_camera.Nv, 3, pixel_data, stride_bytes);
            break;   
        }
        case GL::IMAGE_TYPES::JPG :
        {
            status = stbi_write_jpg(filename.c_str(), m_camera.Nu, m_camera.Nv, 3, pixel_data, stride_bytes);
            break;   
        }
        case GL::IMAGE_TYPES::BMP :
        {
            status = stbi_write_bmp(filename.c_str(), m_camera.Nu, m_camera.Nv, 3, pixel_data);
            break;   
        }
        case GL::IMAGE_TYPES::TGA :
        {
            status = stbi_write_tga(filename.c_str(), m_camera.Nu, m_camera.Nv, 3, pixel_data);
            break;   
        }
        default:{
            std::cout << "Unsupported image type for screenshot. Supported types are {png, jpg, bmp, tga}..." << std::endl;
            break;
        }
    }
    delete[] pixel_data;
}

void GL::SetAlphaNearFarPlane(float alpha){
    alphaNearFarPlane = alpha;
}

// utility function for loading a 2D texture from file
unsigned int GL::LoadTexture(char const * path){
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

float* GL::LoadSTL(cad::part part){
    
    float R = part.color.r;
    float G = part.color.g;
    float B = part.color.b;
    
    float mm2m = 1.0f/1000.0f;  // convert STL from [mm] to [m]
    
    // 3 triangles
    // 9 attributes per triangle (xyz,normal,rgb)
    float *verticies = new float[BUFFER_SIZE_VERTEX * 9 * 3];

    int count = 0;
    for (auto t : part.triangles){        
        verticies[count +  0] = (float) t.v1.x * mm2m;
        verticies[count +  1] = (float) t.v1.y * mm2m;
        verticies[count +  2] = (float) t.v1.z * mm2m;
        verticies[count +  3] = (float) t.normal.x;
        verticies[count +  4] = (float) t.normal.y;
        verticies[count +  5] = (float) t.normal.z;
        verticies[count +  6] = R;
        verticies[count +  7] = G;
        verticies[count +  8] = B;
                
        count = count + 9;
        verticies[count +  0] = (float) t.v2.x * mm2m;
        verticies[count +  1] = (float) t.v2.y * mm2m;
        verticies[count +  2] = (float) t.v2.z * mm2m;
        verticies[count +  3] = (float) t.normal.x;
        verticies[count +  4] = (float) t.normal.y;
        verticies[count +  5] = (float) t.normal.z;
        verticies[count +  6] = R;
        verticies[count +  7] = G;
        verticies[count +  8] = B;
        
        count = count + 9;
        verticies[count +  0] = (float) t.v3.x * mm2m;
        verticies[count +  1] = (float) t.v3.y * mm2m;
        verticies[count +  2] = (float) t.v3.z * mm2m;
        verticies[count +  3] = (float) t.normal.x;
        verticies[count +  4] = (float) t.normal.y;
        verticies[count +  5] = (float) t.normal.z;
        verticies[count +  6] = R;
        verticies[count +  7] = G;
        verticies[count +  8] = B;
        
        count = count + 9;
    }
    
    return verticies;
}

CAD GL::LoadCAD(
        const std::string& root_dir, 
        const std::string& fn_csv, 
        const std::string& fn_textureDiffuse, 
        const std::string& fn_textureSpecular,
        float scale)
{
    
    // load assembly
    CAD foo;
    try {
        foo.assembly = cad::parse(fn_csv, root_dir);
        foo.VAO = new GLuint[BUFFER_SIZE_PARTS];
        foo.VBO = new GLuint[BUFFER_SIZE_PARTS];
        foo.texture.diffuse = LoadTexture(fn_textureDiffuse.c_str());
        foo.texture.specular = LoadTexture(fn_textureSpecular.c_str());
        foo.r_vbs = Vector(3);
        foo.q_vbs2body = Vector(4);
        foo.q_vbs2body(0) = 1;
        foo.scale = scale;
        foo.initialized = true;
    }catch (...){
        std::cout << "Error parsing CSV: "<< std::string( root_dir + fn_csv ) << std::endl;
        throw std::runtime_error("Could not read CSV\n");
        return foo;
    }
    int N_parts = foo.assembly.parts.size();
    
    //printf("N_parts = %d\n", N_parts);
    
    int total_triangles = 0;
    
    // first, configure the cube's VAO (and VBO_parts)
    glGenVertexArrays(BUFFER_SIZE_PARTS, foo.VAO);
    glGenBuffers(BUFFER_SIZE_PARTS, foo.VBO);

    float* vertices;
    int count = 0;
    for (auto part : foo.assembly.parts){
        // determine number of triangles are in part
        vertices = LoadSTL( part );
        int N = part.triangles.size();
        
        // load data into buffer
        glBindBuffer(GL_ARRAY_BUFFER, foo.VBO[count]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*27*N, vertices, GL_STATIC_DRAW);

        // specify format [position, normals, rgb]
        float STRIDE = (3 + 3 + 3) * sizeof(float);
        glBindVertexArray(foo.VAO[count]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
                
        count++;
        
        delete[] vertices;
        
        total_triangles = total_triangles + N;
        
        if( count == N_parts )
            break;
    }

    //printf("total_triangles = %d\n", total_triangles);
    
    return foo;    
}
CAD GL::LoadTexturedSphere(
        const std::string& root_dir, 
        const std::string& fn_csv, 
        const std::string& fn_textureDiffuse, 
        const std::string& fn_textureSpecular,
        float scale)
{    
    // load assembly
    CAD foo;
    try {
        foo.assembly = cad::parse(fn_csv, root_dir);
        foo.VAO = new GLuint[BUFFER_SIZE_PARTS];
        foo.VBO = new GLuint[BUFFER_SIZE_PARTS];
        foo.texture.diffuse = LoadTexture(fn_textureDiffuse.c_str());
        foo.texture.specular = LoadTexture(fn_textureSpecular.c_str());
        foo.r_vbs = Vector(3);
        foo.q_vbs2body = Vector(4);
        foo.q_vbs2body(0) = 1;
        foo.scale = scale;
        foo.initialized = true;
    }catch (...){
        std::cout << "Error parsing CSV: "<< std::string( root_dir + fn_csv ) << std::endl;
        throw std::runtime_error("Could not read CSV\n");
        return foo;
    }
    int N_parts = foo.assembly.parts.size();
    
    // first, configure the cube's VAO (and VBO_parts)
    glGenVertexArrays(BUFFER_SIZE_PARTS, foo.VAO);
    glGenBuffers(BUFFER_SIZE_PARTS, foo.VBO);
    
    // 3 triangles
    // 8 attributes per triangle (xyz,normal,uv)
    float vertices[BUFFER_SIZE_VERTEX * 8 * 3];
    int count = 0;
    for (auto part : foo.assembly.parts){

        // determine number of triangles are in part
        float mm2m = 1.0f/1000.0f;  // convert STL from [mm] to [m]
        float epsilon = 1e-1;
        float latitude, longitude, xy_norm, u, v;
        int count_triangles = 0;
        for (auto t : part.triangles){        
            
            //mexPrintf("count_triangles = %05d / %05d = %f\n", count_triangles, BUFFER_SIZE_VERTEX, count_triangles*100.0/BUFFER_SIZE_VERTEX );
            // avoid singularity
            t.v1.x += epsilon; t.v1.y += epsilon; t.v1.z += epsilon;
            t.v2.x += epsilon; t.v2.y += epsilon; t.v2.z += epsilon;
            t.v3.x += epsilon; t.v3.y += epsilon; t.v3.z += epsilon;
            
            vertices[count_triangles +  0] = (float) t.v1.x * mm2m;
            vertices[count_triangles +  1] = (float) t.v1.y * mm2m;
            vertices[count_triangles +  2] = (float) t.v1.z * mm2m;
            vertices[count_triangles +  3] = (float) t.normal.x;
            vertices[count_triangles +  4] = (float) t.normal.y;
            vertices[count_triangles +  5] = (float) t.normal.z;
            xy_norm = sqrt(t.v1.x*t.v1.x + t.v1.y*t.v1.y);
            latitude = std::atan2(t.v1.z, xy_norm) * RAD2DEG;
            longitude = std::atan2(t.v1.y, t.v1.x) * RAD2DEG;
            v = 0.5 - latitude/180.0;
            u = longitude/360.0 + 0.5;
            vertices[count_triangles +  6] = u;
            vertices[count_triangles +  7] = v;

            count_triangles = count_triangles + 8;
            vertices[count_triangles +  0] = (float) t.v2.x * mm2m;
            vertices[count_triangles +  1] = (float) t.v2.y * mm2m;
            vertices[count_triangles +  2] = (float) t.v2.z * mm2m;
            vertices[count_triangles +  3] = (float) t.normal.x;
            vertices[count_triangles +  4] = (float) t.normal.y;
            vertices[count_triangles +  5] = (float) t.normal.z;
            xy_norm = sqrt(t.v2.x*t.v2.x + t.v2.y*t.v2.y);
            latitude = std::atan2(t.v2.z, xy_norm) * RAD2DEG;
            longitude = std::atan2(t.v2.y, t.v2.x) * RAD2DEG;
            v = 0.5 - latitude/180.0;
            u = longitude/360.0 + 0.5;
            vertices[count_triangles +  6] = u;
            vertices[count_triangles +  7] = v;

            count_triangles = count_triangles + 8;
            vertices[count_triangles +  0] = (float) t.v3.x * mm2m;
            vertices[count_triangles +  1] = (float) t.v3.y * mm2m;
            vertices[count_triangles +  2] = (float) t.v3.z * mm2m;
            vertices[count_triangles +  3] = (float) t.normal.x;
            vertices[count_triangles +  4] = (float) t.normal.y;
            vertices[count_triangles +  5] = (float) t.normal.z;
            xy_norm = sqrt(t.v3.x*t.v3.x + t.v3.y*t.v3.y);
            latitude = std::atan2(t.v3.z, xy_norm) * RAD2DEG;
            longitude = std::atan2(t.v3.y, t.v3.x) * RAD2DEG;
            v = 0.5 - latitude/180.0;
            u = longitude/360.0 + 0.5;
            vertices[count_triangles +  6] = u;
            vertices[count_triangles +  7] = v;

            count_triangles = count_triangles + 8;
        }
        
        // load data into buffer
        int N = part.triangles.size();
        float STRIDE_VBO = 3 * (3 + 3 + 2) * sizeof(float);
        glBindBuffer(GL_ARRAY_BUFFER, foo.VBO[count]);
        glBufferData(GL_ARRAY_BUFFER, STRIDE_VBO*N, vertices, GL_STATIC_DRAW);

        // specify format [position, normals, uv]
        float STRIDE_VAO = (3 + 3 + 2) * sizeof(float);
        glBindVertexArray(foo.VAO[count]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_VAO, (void*)0);                     // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE_VAO, (void*)(3 * sizeof(float)));   // normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE_VAO, (void*)(6 * sizeof(float)));   // uv
        glEnableVertexAttribArray(2);
                
        count++;
        
        if( count == N_parts )
            break;
    }

    return foo;    
}

void GL::setTriadState(bool triadOn)
{
    m_triad.on = triadOn;
}

void GL::DeleteCAD(CAD& cad){

    if (cad.initialized == true){
        delete cad.VAO;
        delete cad.VBO;
        cad.initialized = false;
    }
}

glm::vec3 GL::VBS2GL(Vector& r_vbs){
    // r_vbs
    // +x = right
    // +y = down
    // +z = outward

    // r_gl
    // +x = right
    // +y = up
    // +z = inward

    glm::vec3 r_gl(r_vbs(0), -1.0*r_vbs(1), -1.0*r_vbs(2));
    return r_gl;
}

void GL::VertexShader(CAD& cad, float d_near_gl, float d_far_gl){

    // saturate at 10 [cm]
    if (d_near_gl < 0.1)
        d_near_gl = 0.1;

    glm::mat4 projection = glm::perspective(glm::radians(m_camera.FOV_vertical_deg), (float)m_camera.Nu / (float)m_camera.Nv, d_near_gl, d_far_gl);
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 model;
    cad.shader.setMat4("projection", projection);
    cad.shader.setMat4("view", view);
    cad.shader.setMat4("model", model);
}

void GL::FragmentShader(CAD& cad){
    // be sure to activate shader when setting uniforms/drawing objects
    cad.shader.use();        
    cad.shader.setVec3("r_Go2Vo_gl", m_camera.Position);
    cad.shader.setFloat("material.shininess", 32.0f);
    cad.shader.setInt("material.diffuse", 0);
    cad.shader.setInt("material.specular", 1);

    // directional light (Sun)
    if (m_sun.initialized && m_sun.on ){
        glm::vec3 r_Vo2So_gl = VBS2GL(m_sun.r_vbs);
        cad.shader.setVec3("sun.r_Go2Lo_gl", r_Vo2So_gl);
        cad.shader.setVec3("sun.ambient", 0.05f, 0.05f, 0.05f);
        cad.shader.setVec3("sun.diffuse", 0.4f, 0.4f, 0.4f);
        cad.shader.setVec3("sun.specular", 0.5f, 0.5f, 0.5f);
    }

    // directional light (Moon)
    if (m_moon.initialized && m_moon.on ){
        glm::vec3 r_Vo2Mo_gl = VBS2GL(m_moon.r_vbs);
        cad.shader.setVec3("moon.r_Go2Lo_gl", r_Vo2Mo_gl);
        cad.shader.setVec3("moon.ambient", 0.05f, 0.05f, 0.05f);
        cad.shader.setVec3("moon.diffuse", 0.4f, 0.4f, 0.4f);
        cad.shader.setVec3("moon.specular", 0.5f, 0.5f, 0.5f);
    }

    // directional light (Lamp)
    if (m_lamp.initialized && m_lamp.on ){
        glm::vec3 r_Vo2Lo_gl = VBS2GL(m_lamp.r_vbs);
        cad.shader.setVec3("sun.r_Go2Lo_gl", r_Vo2Lo_gl);
        cad.shader.setVec3("sun.ambient", 0.05f, 0.05f, 0.05f);
        cad.shader.setVec3("sun.diffuse", 0.4f, 0.4f, 0.4f);
        cad.shader.setVec3("sun.specular", 0.5f, 0.5f, 0.5f);
    }
}

void GL::DrawCAD(CAD& cad){

    if(cad.initialized == false || cad.on == false)
        return;

    FragmentShader(cad);    
    float d_near_vbs = Norm(cad.r_vbs) - alphaNearFarPlane*cad.scale;
    float d_far_vbs = d_near_vbs + 2*alphaNearFarPlane*cad.scale;
    VertexShader(cad, d_near_vbs, d_far_vbs);
    
    // bind diffuse map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cad.texture.diffuse);

    // bind specular map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cad.texture.specular);

    // render containers
    int count = 0;
    for(auto part : cad.assembly.parts){
        
        glBindVertexArray(cad.VAO[count]);
        
        // calculate the model matrix for each object and pass it to shader before drawing
        Vector anglevec = Quaternion2AngleVec(cad.q_vbs2body);
        float angle_deg = anglevec(0) * RAD2DEG;
        glm::vec3 r_gl = VBS2GL(cad.r_vbs);
        glm::mat4 model;
        model = glm::translate(model, r_gl);
        if( angle_deg != 0)
            model = glm::rotate(model, glm::radians(angle_deg), glm::vec3(anglevec(1), anglevec(2), anglevec(3)));
        model = glm::scale(model, glm::vec3(cad.scale));
        cad.shader.setMat4("model", model);

        //glDrawArrays(GL_TRIANGLES, 0, 36);
        glDrawArrays(GL_TRIANGLES, 0, 3*part.triangles.size());
        
        count++;
    }
}

void GL::DrawRGBStar(Vector& n_vbs, Vector& rgb){
    // also draw the lamp object(s)
    //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screen_width_pix / (float)screen_height_pix, 0.1f, 100.0f);;
    glm::mat4 projection = glm::perspective(glm::radians(m_camera.FOV_vertical_deg), (float)m_camera.Nu / (float)m_camera.Nv, 0.1f, 100.0f);
    glm::mat4 view = m_camera.GetViewMatrix();
    m_star.shader.use();
    m_star.shader.setMat4("projection", projection);
    m_star.shader.setMat4("view", view);

    float d = 50.0;
    int count = 0;
    for(auto part : m_star.assembly.parts){
        glBindVertexArray(m_star.VAO[count]);

        // coordinate transformation
        glm::vec3 r_gl = VBS2GL(d*n_vbs);

        glm::mat4 model;
        model = glm::translate(model, r_gl);
        model = glm::scale(model, glm::vec3(m_star.scale)); 
        m_star.shader.setMat4("model", model);

        // radiometric mapping
        glm::vec3 rgb(rgb(0), rgb(1), rgb(2));
        m_star.shader.setVec3("RGB", rgb);
        
        //glDrawArrays(GL_TRIANGLES, 0, 36);
        glDrawArrays(GL_TRIANGLES, 0, 3*part.triangles.size());

        count++;
    }
}


void GL::RenderDots(const Matrix& n_vbs, const Matrix& rgb){

    // reset screen
    mouse_input = false;
    m_camera.Reset();
    ClearScreen();

    // render stars    
    for(int i=0; i<n_vbs.nRows(); i++){
        DrawRGBStar(n_vbs.Row(i), rgb.Row(i));
    }
        
    // swap buffers
    glfwSwapBuffers(m_window);
    
}


/*

void GL::AnalyzeCAD(SP3& sp3){
    
    mouse_input = true;
    firstMouse = true;
    camera.Reset();
    while (!glfwWindowShouldClose(window)){
        
        // input
        ProcessInput();

        // reset screen
        ClearScreen();

        // Draw TANGO
        tango.r_vbs = sp3.r_Vo2To_vbs;
        tango.q_vbs2body = sp3.q_vbs2tango;
        Draw(tango, true);
        
        // Draw Earth
        //earth.r_vbs = sp3.r_Eo2Mcm_eci;
        float z = 100 * 1000 * 1000;
        earth.r_vbs = Vector(0, 0, z);
        earth.q_vbs2body = Vector(1,0,0,0);
        Draw(earth, true);
        
        // Draw Sun
        sun.r_vbs = sp3.r_Eo2So_vbs;
        sun.q_vbs2body = Vector(1,0,0,0);
        Draw(sun, true);

        // Draw Moon
        moon.r_vbs = sp3.r_Eo2Mo_vbs;
        moon.q_vbs2body = Vector(1,0,0,0);
        Draw(moon, true);
        
        // Draw TANGO triad
        triad.r_vbs = sp3.r_Vo2To_vbs;
        triad.q_vbs2body = sp3.q_vbs2tango;
        Draw(triad, true);

        // Draw GL triad
        triad.r_vbs = Vector(3);
        triad.q_vbs2body = Vector(1,0,0,0);
        Draw(triad, true);
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    mouse_input = false;
    
    Terminate();
}

void GL::AnalyzeEarth(Vector& r_Vo2Eo_vbs, Vector& q_vbs2ecef){
    
    mouse_input = true;
    firstMouse = true;
    camera.Reset();
    while (!glfwWindowShouldClose(window)){
        
        // input
        ProcessInput();

        // reset screen
        ClearScreen();

        // Draw Sun
        sun.r_vbs = Vector(0,0,0);
        sun.q_vbs2body = Vector(1,0,0,0);
        Draw(sun, true);

        // render Earth
        earth.r_vbs = r_Vo2Eo_vbs;
        earth.q_vbs2body = q_vbs2ecef;
        Draw(earth, true);

        // add triad
        triad.r_vbs = r_Vo2Eo_vbs;
        triad.q_vbs2body = q_vbs2ecef;
        Draw(triad, true);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    mouse_input = false;
    Terminate();
}

void GL::AnalyzeShader(Vector& r_Vo2Co_vbs, Vector& q_vbs2body, Vector& r_Vo2So_vbs){
    
    mouse_input = true;
    firstMouse = true;
    camera.Reset();
    while (!glfwWindowShouldClose(window)){
        
        // input
        ProcessInput();

        // reset screen
        ClearScreen();
        
        // Draw Sun
        sun.r_vbs = r_Vo2So_vbs;
        sun.q_vbs2body = q_vbs2body;
        Draw(sun);
        
        // Draw Cube
        cube.r_vbs = r_Vo2Co_vbs;
        cube.q_vbs2body = q_vbs2body;
        Draw(cube);
        
        // Draw GL (World) Triad
        triad.r_vbs = Vector(0,0,0);
        triad.q_vbs2body = Vector(1,0,0,0);
        Draw(triad);
        
        // Draw Cube Triad
        triad.r_vbs = r_Vo2Co_vbs; 
        triad.q_vbs2body = q_vbs2body;
        Draw(triad);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    mouse_input = false;
    Terminate();
}
*/

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void GL::ProcessInput(){

    // per-frame time logic
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // keyboard input
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.ProcessKeyboard(RIGHT, deltaTime);
}



void CallbackFrameBufferSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void CallbackMouse(GLFWwindow* window, double xpos, double ypos)
{
    if (mouse_input == false)
        return;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    
    float K_mouse = 0.2;
    xoffset = K_mouse * xoffset;
    yoffset = K_mouse * yoffset;

    lastX = xpos;
    lastY = ypos;

    //camera.ProcessMouseMovement(xoffset, yoffset);
}

void CallbackScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(yoffset);
}

static void CallbackKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        //glfwSetWindowShouldClose(window, GLFW_TRUE);
        glfwTerminate();
    }
}

void CallbackWindowClose(GLFWwindow* window){
    //glfwSetWindowShouldClose(window, GLFW_FALSE);
    glfwTerminate();
}