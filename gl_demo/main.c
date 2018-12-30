// Sources:
// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// https://stackoverflow.com/questions/13403807/glvertexattribpointer-raising-gl-invalid-operation

#define GL_GLEXT_PROTOTYPES 1

#include <GL/gl.h>
#include <gtk/gtk.h>

static GLfloat vertices[] = {
  // Front face.
  -0.6f, 0.6f, -1.0f,
  -0.6f, 0.1f, -1.0f,
  -0.1f, 0.1f, -1.0f,
  -0.1f, 0.1f, -1.0f,
  -0.1f, 0.6f, -1.0f,
  -0.6f, 0.6f, -1.0f,

  // Back face.
  -0.6f, 0.6f, -1.5f,
  -0.6f, 0.1f, -1.5f,
  -0.1f, 0.1f, -1.5f,
  -0.1f, 0.1f, -1.5f,
  -0.1f, 0.6f, -1.5f,
  -0.6f, 0.6f, -1.5f,

  // Right face.
  -0.1f, 0.1f, -1.0f,
  -0.1f, 0.1f, -1.5f,
  -0.1f, 0.6f, -1.5f,
  -0.1f, 0.6f, -1.5f,
  -0.1f, 0.6f, -1.0f,
  -0.1f, 0.1f, -1.0f,

  // Left face.
  -0.6f, 0.1f, -1.0f,
  -0.6f, 0.1f, -1.5f,
  -0.6f, 0.6f, -1.5f,
  -0.6f, 0.6f, -1.5f,
  -0.6f, 0.6f, -1.0f,
  -0.6f, 0.1f, -1.0f,

  // Bottom face.
  -0.6f, 0.1f, -1.0f,
  -0.1f, 0.1f, -1.0f,
  -0.1f, 0.1f, -1.5f,
  -0.1f, 0.1f, -1.5f,
  -0.6f, 0.1f, -1.5f,
  -0.6f, 0.1f, -1.0f,

  // Top face.
  -0.6f, 0.6f, -1.0f,
  -0.1f, 0.6f, -1.0f,
  -0.1f, 0.6f, -1.5f,
  -0.1f, 0.6f, -1.5f,
  -0.6f, 0.6f, -1.5f,
  -0.6f, 0.6f, -1.0f,
};

static GLfloat normals[] = {
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,

  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,

  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,

  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,

  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,

  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
};

static GLuint program;
static GLuint vao;
static GLuint vertex_buffer;
static GLuint normal_buffer;

static GLuint load_shaders() {
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	const char* vertex_shader_code = "\
    #version 330 core\n \
    layout (location = 0) in vec3 pos; \n \
    layout (location = 1) in vec3 normal; \n \
    varying vec3 vertex_normal; \n \
    void main() { \n \
      gl_Position.xyz = pos; \n \
      gl_Position.z = pos.z * pos.z / 10; \n \
      gl_Position.w = -pos.z; \n \
      vertex_normal = normal; \n \
    }";

  const char* fragment_shader_code = "\
    #version 330 core \n \
    varying vec3 vertex_normal; \n \
    out vec3 color; \n \
    void main() { \n \
      float bn = dot(vertex_normal, normalize(vec3(0.3, 0.4, 0.5))); \n \
      if (bn < 0) { \n \
        bn = -bn; \n \
      } \n \
      color = vec3(bn, 0, 0); \n \
    }";

	GLint result = GL_FALSE;

	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
	if (!result) {
    goto fail;
	}

	glShaderSource(fragment_shader, 1, &fragment_shader_code , NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
	if (!result) {
		goto fail;
	}

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (!result) {
		goto fail;
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;

fail:
  glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
  glDeleteProgram(program);

  printf("failed to create shaders.\n");
  exit(1);
}

static gboolean render(GtkGLArea* area, GdkGLContext* ctx) {
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearDepth(1.0);

  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 20.0f);
  glMatrixMode(GL_MODELVIEW);

  glUseProgram(program);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  return TRUE;
}

static void realize(GtkGLArea* area) {
	gtk_gl_area_make_current(area);
  gtk_gl_area_set_has_depth_buffer(area, TRUE);

  program = load_shaders();

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &normal_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
}

static void activate(GtkApplication* app, gpointer userData) {
  GtkWidget* window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "GL Demo");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

  GtkWidget* gl_area = gtk_gl_area_new();
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  gtk_container_add(GTK_CONTAINER(window), gl_area);

  gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
  GtkApplication* app = gtk_application_new("com.aqnichol.button_catcher",
                                            G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
