#ifndef OBJECT_H
#define OBJECT_H

#include<iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>



class Object
{
public:

	int numVertices;

	GLuint VBO, VAO;

	glm::mat4 model = glm::mat4(1.0);


	Object(const char* path) {

		// Read the file defined by the path argument 
		// open the .obj file into a text editor and see how the data are organized
		// you will see line starting by v, vt, vn and f --> what are these ?
		// Then parse it to extract the data you need
		// keep track of the number of vertices you need

		numVertices; // = ...
	}



	void makeObject(Shader shader, bool texture = true) {


		//Create the VAO and VBO
		//Put your data into your VBO
		//Define VBO and VAO as active buffer and active vertex array
		//Use the VAO to specify how your data should be read by your shader (glVertexAttribPointer and co)
		//Sometimes your shader will not use texture or normal attribute
		//you can use the boolean defined above to not specify these attribute 
		//desactive the buffer and delete the datas when your done


	}

	void draw() {

		//bind your vertex arrays and call glDrawArrays

	}
};
#endif