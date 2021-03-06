#include "Renderer.h"

const GLchar* vertexSource =
  "#version 150 core\n"
  "in vec2 position;"
  "in vec3 color;"
  "in vec2 texcoord;"
  "out vec3 Color;"
  "out vec2 Texcoord;"
  "void main()"
  "{"
  "    Color = color;"
  "    Texcoord = texcoord;"
  "    gl_Position = vec4(position, 0.0, 1.0);"
  "}";
const GLchar* fragmentSource =
  "#version 150 core\n"
  "in vec3 Color;"
  "in vec2 Texcoord;"
  "out vec4 outColor;"
  "uniform sampler2D tex;"
  "void main()"
  "{"
  "    outColor = texture(tex, Texcoord);"
  "}";

Renderer* current_renderer;

void redraw_global() {
  if (current_renderer->_dec.done) {
    glutLeaveMainLoop();
  }
  std::vector<uint8_t> vf = current_renderer->_dec.get_video_frame().data;

  if (vf.size() == 0) {return;}

  for (int i=0; i<current_renderer->get_num_windows(); ++i) {
    glutSetWindow(current_renderer->get_window_id(i));
    glFlush();
    
    int & window_width = 
      current_renderer->_windows[i].window_width;
    int & window_height = 
      current_renderer->_windows[i].window_height;
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, current_renderer->_dec.get_width(), current_renderer->_dec.get_height(), GL_RGB, GL_UNSIGNED_BYTE, vf.data());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glutSwapBuffers();
    if (glutGet(GLUT_WINDOW_WIDTH)  != window_width ||
        glutGet(GLUT_WINDOW_HEIGHT) != window_height) {
      current_renderer->_windows[i].update_section();     
      
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDepthMask(GL_TRUE);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  }
}

void keyboard_global(unsigned char key, int x, int y) {
  if( key == 'q' || key == 27 ) {
    current_renderer->_dec.stop();
    glutLeaveMainLoop();
  } else if (key == 'F') {
    glutFullScreen();
  } else if (key == 'f') {
    glutReshapeWindow(480,270);
  } else if (key == 's') {
    current_renderer->_dec.seek(20);
  } else if (key == 'S') {
    current_renderer->_dec.seek(-20);
  }
}


GLWindow::GLWindow(int video_width, int video_height, std::string window_name,
  float section_left, float section_right, 
  float section_top, float section_bottom) : 
  _video_width(video_width), _video_height(video_height) {
  //std::cout << "trying to create a window" << std::endl;

  vertices[6]   = section_top;
  vertices[13]  = section_top;
  vertices[20]  = section_bottom;
  vertices[27]  = section_bottom;
  vertices[5]   = section_left;
  vertices[26]  = section_left;
  vertices[12]  = section_right;
  vertices[19]  = section_right;

  float image_section_height = section_bottom - section_top;
  float image_section_width  = section_right  - section_left;

  desired_aspect_ratio = (video_width *image_section_width) / 
                         (video_height*image_section_height);

  bound_top    = (section_top    != 0);
  bound_bottom = (section_bottom != 1);
  bound_left   = (section_left   != 0);
  bound_right  = (section_right  != 1);

  glutInitDisplayMode(GLUT_RGB);
  glutInitWindowSize(video_width,video_height); // width=400pixels height=500pixels

  id = glutCreateWindow(window_name.c_str());  // create window

  // Initialise glew
  glewExperimental = GL_TRUE; //needed as it is old!
  GLenum err = glewInit();
  if (GLEW_OK != err) {
     std::cerr<<"Error: "<<glewGetErrorString(err)<<std::endl;
  } else {
     if (GLEW_VERSION_3_3)
     {
      //std::cout<<"Driver supports OpenGL 3.3:"<<std::endl;
     }
  }
  
  glClearColor(0.0,0.0,0.0,0.0);  // set background to black
  glutDisplayFunc(redraw_global); // set window's display callback
  glutIdleFunc(redraw_global);
  glutKeyboardFunc(keyboard_global); // set window's key callback

  // Set closing behaviour: If we leave the mainloop 
  //(e.g. through user input or closing the window) 
  //we continue after the function "glutMainLoop()"
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);


  // Create Vertex Array Object
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create a Vertex Buffer Object and copy the vertex data to it
  GLuint vbo;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Create an element array
  GLuint ebo;
  glGenBuffers(1, &ebo);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

  // Create and compile the vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  // Create and compile the fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // Link the vertex and fragment shader into a shader program
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor");
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  // Specify the layout of the vertex data
  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), 0);

  GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
  glEnableVertexAttribArray(colAttrib);
  glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

  GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));

  // Load texture
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  //set pixels array as texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _video_width, _video_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //std::cout << "window created\n";
}

GLWindow::GLWindow(int video_width, int video_height, WindowConfig w) : 
  GLWindow(video_width, video_height, w.name, w.x_begin, w.x_end, w.y_begin, w.y_end) {}

GLWindow::~GLWindow() {
  glDeleteTextures(1, &tex);

  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);

  glDeleteBuffers(1, &ebo);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
}

void GLWindow::update_section() {
  window_width  = glutGet(GLUT_WINDOW_WIDTH);
  window_height = glutGet(GLUT_WINDOW_HEIGHT);

  double current_ap = static_cast<double>(window_width) / 
                      static_cast<double>(window_height);

  if (current_ap > desired_aspect_ratio) {
    //coordinates go from -1 to 1
    float width_ratio =
      2*window_height * desired_aspect_ratio / window_width;

    if (bound_left && !bound_right) {
      //image has to start left, shift it by -1
      vertices[0]  = -1;
      vertices[7]  = width_ratio-1;
      vertices[14] = width_ratio-1;
      vertices[21] = -1;
    } else if (bound_right && !bound_left) {
      //image has to start right, shift it
      vertices[0]  = 1-width_ratio;
      vertices[7]  = 1;
      vertices[14] = 1;
      vertices[21] = 1-width_ratio;
    } else {
      //image needs to be in the middle:
      float shift = (2 - width_ratio) / 2;

      vertices[0]  = (1-shift) - width_ratio;
      vertices[7]  = width_ratio - 1 + shift;
      vertices[14] = width_ratio - 1 + shift;
      vertices[21] = (1-shift) - width_ratio;
    }
    
    //image needs to go from bottom to top;
    vertices[1]  =  1;
    vertices[8]  =  1;
    vertices[15] = -1;
    vertices[22] = -1;
  } else {
    float height_ratio = 2*window_width / desired_aspect_ratio
                         / window_height;
    
    if (bound_top && !bound_bottom) {
      vertices[1]  = 1;
      vertices[8]  = 1;
      vertices[15] = 1-height_ratio;
      vertices[22] = 1-height_ratio;
    } else if (bound_bottom && !bound_top) {
      vertices[1]  = height_ratio-1;
      vertices[8]  = height_ratio-1;
      vertices[15] = -1;
      vertices[22] = -1;
    } else {
      float shift = (2 - height_ratio) / 2;
      vertices[1] = height_ratio - 1 + shift;
      vertices[8] = height_ratio - 1 + shift;
      vertices[15]  = (1-shift) - height_ratio;
      vertices[22]  = (1-shift) - height_ratio;
    }
    vertices[0]  = -1;
    vertices[7]  =  1;
    vertices[14] =  1;
    vertices[21] = -1;
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

/*****************************************************
* RENDERER
*****************************************************/
Renderer::Renderer(Decoder &dec) : _dec(dec) {
  current_renderer = this;
  int argc = 1;
  char* argv[] = {"egal"};
  glutInit(&argc, argv);  // initialize GLUT system

  _windows.push_back(GLWindow(_dec.get_width(), _dec.get_height(),"GLV",0,1,0,1));
}

Renderer::Renderer(Decoder &dec, const std::vector<WindowConfig> & windows) : _dec(dec) {
  current_renderer = this;
  int argc = 1;
  char* argv[] = {"egal"};
  glutInit(&argc, argv);

  for (auto & w : windows) {
    _windows.push_back(GLWindow(_dec.get_width(), _dec.get_height(), w));
  }
}

Renderer::~Renderer() {
  std::cout << "Renderer is destroyed\n";
}

std::vector<std::string> split_string
  (std::string s, const std::string & delimiter) {
  std::vector<std::string> result;

  size_t pos = 0;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    result.emplace_back(s.substr(0,pos));
    s.erase(0,pos + delimiter.length());
  }

  if (s.size() > 0) {
    result.emplace_back(s);
  }

  return result;
}


void Renderer::run() {
  glutMainLoop();
}

void Renderer::draw(std::vector<uint8_t> frame) {
  if (_dec.done) {
    return;
  }
  
  if (frame.size() == 0) {return;}

  for (int i=0; i<_windows.size(); ++i) {
    glutSetWindow(get_window_id(i));
    glFlush();
    
    int & window_width = _windows[i].window_width;
    int & window_height = _windows[i].window_height;
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _dec.get_width(), _dec.get_height(), 
    GL_RGB, GL_UNSIGNED_BYTE, frame.data());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//    glutSwapBuffers();
    
    if (glutGet(GLUT_WINDOW_WIDTH)  != window_width ||
        glutGet(GLUT_WINDOW_HEIGHT) != window_height) {
      _windows[i].update_section();     

      glViewport(0, 0, window_width, window_height);

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDepthMask(GL_TRUE);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  }
}


void Renderer::set_fullscreen() {
  for (int i=0; i<_windows.size(); ++i) {
    glutSetWindow(get_window_id(i));
    glutFullScreen();
  }
}

void Renderer::leave_fullscreen() {
  for (int i=0; i<_windows.size(); ++i) {
    glutReshapeWindow(_dec.get_width(), _dec.get_height());
  }
}

void Renderer::keyboard(unsigned char key, int x, int y) {
  if( key == 'q' || key == 27 ) glutLeaveMainLoop();
}

int Renderer::get_num_windows() {
  return _windows.size();
}

int Renderer::get_window_id(int win) {
  return _windows[win].id;
}
