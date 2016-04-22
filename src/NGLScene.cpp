#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <QTimer>

//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;

NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in that case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0.0f;
  m_spinYFace=0.0f;
  setTitle("Qt5 Simple NGL Demo");


}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(QResizeEvent *_event)
{
  m_width=static_cast<int>(_event->size().width()*devicePixelRatio());
  m_height=static_cast<int>(_event->size().height()*devicePixelRatio());
  // now set the camera size values as the screen size has changed
  m_cam.setShape(45.0f,static_cast<float>(width())/height(),0.05f,350.0f);
}

void NGLScene::resizeGL(int _w , int _h)
{
  m_cam.setShape(45.0f,static_cast<float>(_w)/_h,0.05f,350.0f);
  m_width=static_cast<int>(_w*devicePixelRatio());
  m_height=static_cast<int>(_h*devicePixelRatio());
}


void NGLScene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::instance();
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
#ifndef USINGIOS_
  glEnable(GL_MULTISAMPLE);
#endif
   // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  // we are creating a shader called Phong to save typos
  // in the code create some constexpr
  constexpr auto shaderProgram="Phong";
  constexpr auto vertexShader="PhongVertex";
  constexpr auto fragShader="PhongFragment";
  // create the shader program
  shader->createShaderProgram(shaderProgram);
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader(vertexShader,ngl::ShaderType::VERTEX);
  shader->attachShader(fragShader,ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource(vertexShader,"shaders/PhongVertex.glsl");
  shader->loadShaderSource(fragShader,"shaders/PhongFragment.glsl");
  // compile the shaders
  shader->compileShader(vertexShader);
  shader->compileShader(fragShader);
  // add them to the program
  shader->attachShaderToProgram(shaderProgram,vertexShader);
  shader->attachShaderToProgram(shaderProgram,fragShader);


  // now we have associated that data we can link the shader
  shader->linkProgramObject(shaderProgram);
  // and make it active ready to load values
  (*shader)[shaderProgram]->use();
  // the shader will use the currently active material and light0 so set them
  ngl::Material m(ngl::STDMAT::GOLD);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,5,5);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  // now load to our new camera
  m_cam.set(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape(45.0f,720.0f/576.0f,0.05f,350.0f);
  shader->setUniform("viewerPos",m_cam.getEye().toVec3());
  // now create our light that is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv=m_cam.getViewMatrix();
  iv.transpose();
  ngl::Light light(ngl::Vec3(-2,5,2),ngl::Colour(1,1,1,1),ngl::Colour(1,1,1,1),ngl::LightModes::POINTLIGHT );
  light.setTransform(iv);
  // load these values to the shader as well
  light.loadToShader("light");

    startTimer(2);
    factor=0.01;

    m_boxPos.set(2,0,0);
    m_box2Pos.set(-2,0,0);

    m_spherePos.set(0,0,-2);
    sphereRadius=0.1;
    m_sphereVel.set(0.02,0,0.02);

    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    prim->createSphere("sphere", sphereRadius, 40);

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;

  M= m_transform.getMatrix()*m_mouseGlobalTX;
  MV=  M*m_cam.getViewMatrix();
  MVP= M*m_cam.getVPMatrix();
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setShaderParamFromMat4("MV",MV);
  shader->setShaderParamFromMat4("MVP",MVP);
  shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
  shader->setShaderParamFromMat4("M",M);
}


//One way of doing it
/////////////////////////////////////////////////////////
/// \brief NGLScene::BoxIntersectsSphere
/// \param Bmin
/// \param Bmax
/// \param C
/// \param r
/// \return
///

//http://stackoverflow.com/questions/4578967/cube-sphere-intersection-test
bool NGLScene::BoxIntersectsSphere(ngl::Vec3 Bmin, ngl::Vec3 Bmax, ngl::Vec3 C, float r) {
  float r2 = r * r;
  float dmin = 0;
  for(int i = 0; i < 3; i++ )
  {
    if( C[i] < Bmin[i] )
        dmin +=  sqrt( C[i] - Bmin[i] );
    else if ( C[i] > Bmax[i] )
        dmin += sqrt( C[i] - Bmax[i] );
  }

  //overall distance between sphere and cube is less than the distance between the center of the sphere + the BminXYZ, BmaxZYZ
  return dmin <= r2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////


//Second way of doing it
/////////////////////////////////////////////////////////
/// \brief SphereToPlane
/// \param sphereCenter
/// \param sphereRadius
/// \param planePoint
/// \param planePointNormal
/// \return
/////////////////////////////////////////////////////////

static bool SphereToPlane(const ngl::Vec3& sphereCenter, const float& sphereRadius, const ngl::Vec3& planePoint, const ngl::Vec3& planePointNormal)
{
    //Calculate a vector from the point on the plane to the center of the sphere
    ngl::Vec3 vecTemp(sphereCenter - planePoint);

    //Calculate the distance: dot product of the new vector with the plane's normal
    float fDist= vecTemp.dot(planePointNormal);

    if(fDist > sphereRadius )
    {
        //The sphere is not touching the plane
        return false;
    }


    //Else, the sphere is colliding with the plane
    return true;
}


#define MAX(x,y) (((x) < (y)) ? (y) : (x))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

static float e=/*0.8*/1; //when e=0 ->  collision is perfectly inelastic, when e=1 ->  collision is perfectly elastic;
static ngl::Vec3 calculateCollisionResponse( ngl::Vec3 & sphereVel, const ngl::Vec3 & normal)
{
    float d = sphereVel.dot (normal);
    float mag= - ( 1 + e ) * d;
    float j = MAX( mag, 0.0 );
    sphereVel += j* normal;
    return sphereVel;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////



/// Test whether a sphere and an axis aligned box intersect each other

/// Minimum coordinate of the axis aligned box to test
/// Maximum coordinate of the axis aligned box to test
/// Center to the sphere to test
/// Radius to the sphere to test
/// True if the axis aligned box intersects with the sphere
///
///   Idea taken from the "Simple Intersection Tests for Games" article
///   on gamasutra by Gomez.
///
bool NGLScene::StaticTest( ngl::Vec3 aabbMin, ngl::Vec3 aabbMax, ngl::Vec3 sphereCenter, double sphereRadius)
{
  ngl::Vec3 closestPointInAabb =MIN(MAX(sphereCenter.length(), aabbMin.length()), aabbMax.length());
  double distanceSquared = (closestPointInAabb - sphereCenter).lengthSquared();

  // The AABB and the sphere overlap if the closest point within the rectangle is
  // within the sphere's radius
  return distanceSquared < (sphereRadius * sphereRadius);
}

bool NGLScene::intersect(ngl::Vec3 sphereCenter, double sphereRadius, ngl::Vec3  boxmin, ngl::Vec3  boxmax)
{
  // get box closest point to sphere center by clamping
  float x = MAX(boxmin.m_x, MIN(sphereCenter.m_x, boxmax.m_x));
  float y = MAX(boxmin.m_y, MIN(sphereCenter.m_y, boxmax.m_y));
  float z = MAX(boxmin.m_z, MIN(sphereCenter.m_z, boxmax.m_z));

  x = (sphereCenter.m_x < boxmin.m_x)? boxmin.m_x : (sphereCenter.m_x > boxmax.m_x)? boxmax.m_x : sphereCenter.m_x;
  y = (sphereCenter.m_y < boxmin.m_y)? boxmin.m_y : (sphereCenter.m_y > boxmax.m_y)? boxmax.m_y : sphereCenter.m_y;
  z = (sphereCenter.m_z < boxmin.m_z)? boxmin.m_z : (sphereCenter.m_z > boxmax.m_z)? boxmax.m_z : sphereCenter.m_z;

  // this is the same as point in sphere
  float distance = ((x - sphereCenter.m_x) * (x - sphereCenter.m_x) +
                           (y - sphereCenter.m_y) * (y - sphereCenter.m_y) +
                           (z - sphereCenter.m_z) * (z - sphereCenter.m_z));

  bool collide=distance < sphereRadius*sphereRadius;
  if(collide)
  {
      std::cout<<"collided"<<std::endl;
  }
  return collide;
}





void NGLScene::paintGL()
{
  glViewport(0,0,m_width,m_height);
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["Phong"]->use();

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  // draw

  m_transform.reset();
  {
      m_transform.setPosition(m_box2Pos);
      loadMatricesToShader();
      prim->draw("cube");
  }


  m_transform.reset();
  {
      m_transform.setPosition(m_boxPos);
      loadMatricesToShader();
      prim->draw("cube");
  }

  m_transform.reset();
  {
      m_transform.setPosition(m_spherePos);
      loadMatricesToShader();
      prim->draw("sphere");
  }


}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // that is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += static_cast<int>( 0.5f * diffy);
    m_spinYFace += static_cast<int>( 0.5f * diffx);
    m_origX = _event->x();
    m_origY = _event->y();
    update();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = static_cast<int>(_event->x() - m_origXPos);
    int diffY = static_cast<int>(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // that method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // that event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//------------------------------m_sphereVel----------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quit
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
#ifndef USINGIOS_

  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
#endif
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  default : break;
  }
 update();
}

#include <ngl/Random.h>

bool NGLScene::checkOutterWalls()
{
    bool collision=false;
    if (m_spherePos.m_x < -3 || m_spherePos.m_x > 3)
    {
        m_sphereVel.m_x= -m_sphereVel.m_x;
        collision=true;
    }
    if (m_spherePos.m_z < -3|| m_spherePos.m_z > 3)
    {
        m_sphereVel.m_z= -m_sphereVel.m_z;
        collision=true;
    }


    return collision;

}

void NGLScene::timerEvent(QTimerEvent * _event)
{

    if(checkOutterWalls())
        std::cout<<"collision happened"<<std::endl;


    float boxwidth=1;
    ngl::Vec3 boxXYZmin(m_boxPos.m_x-boxwidth/2, m_boxPos.m_y-boxwidth/2, m_boxPos.m_z-boxwidth/2);
    ngl::Vec3 boxXYZmaxn(m_boxPos.m_x+boxwidth/2, m_boxPos.m_y+boxwidth/2, m_boxPos.m_z+boxwidth/2);

    ngl::Vec3 box2XYZmin(m_box2Pos.m_x-boxwidth/2, m_box2Pos.m_y-boxwidth/2, m_box2Pos.m_z-boxwidth/2);
    ngl::Vec3 box2XYZmaxn(m_box2Pos.m_x+boxwidth/2, m_box2Pos.m_y+boxwidth/2, m_box2Pos.m_z+boxwidth/2);


    //One way of doing it
//    if (BoxIntersectsSphere(boxXYZmin, boxXYZmaxn, m_spherePos, sphereRadius))
//        //at this stage you need to calculate the normal
//        factor = - factor;
//    m_sphereVel.m_x=factor;



    ngl::Vec3 box1leftwallNormal(-1,0,0);
    ngl::Vec3 box2rightwallNormal(1,0,0);

    //Second way of doing it

//    //right cube
//    if (SphereToPlane(m_spherePos, sphereRadius, ngl::Vec3(boxXYZmin.m_x,0,0), box1leftwallNormal) )
//        m_sphereVel=calculateCollisionResponse(m_sphereVel, box1leftwallNormal);


//    if (SphereToPlane(m_spherePos, sphereRadius, box2XYZmaxn.m_x, box2rightwallNormal))
//         m_sphereVel=calculateCollisionResponse(m_sphereVel, box2rightwallNormal);


//    if ( StaticTest(boxXYZmin, boxXYZmaxn, m_spherePos, sphereRadius) )
    if ( intersect(m_spherePos, sphereRadius,boxXYZmin, boxXYZmaxn)  )
    {
        m_sphereVel = -m_sphereVel;
    }

    if ( intersect(m_spherePos, sphereRadius,box2XYZmin, box2XYZmaxn)  )
    {
        m_sphereVel = -m_sphereVel;
    }


    m_spherePos+=m_sphereVel;


    update();
}
