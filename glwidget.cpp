#include "glwidget.h"
#include <QFileDialog>
//#include <QImage>

GLWidget::GLWidget(QWidget *parent): QOpenGLWidget(parent)
{
    this->setMouseTracking(true);
}


void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.9f,0.9f,1.0f,10.f);
    shader.compile("../Renderer/shader.vert","../Renderer/shader.frag");

    light = QVector3D(0,0,1);

    connect(this,SIGNAL(frameSwapped()),this,SLOT(update()));
    printContext();
}

void GLWidget::resizeGL(int w, int h)
{
    //glViewport( 0, 0, w, h );
    float aspect = w/(float)h;
    camera.setAspect(aspect);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderScene();
}


void GLWidget::renderScene()
{

    //drawLambert();
    shader.program.bind();
    shader.program.setUniformValue("projectionMatrix",camera.projection());
    shader.program.setUniformValue("viewMatrix",camera.view());
    shader.program.setUniformValue("lightDirection",light);

    shader.program.setUniformValue("disptex",m_drawTexture);
    shader.program.setUniformValue("disptexnorm",m_drawTextureLambert);
    shader.program.setUniformValue("dispnorm",m_drawNormals);
    shader.program.setUniformValue("dispalb",m_drawLambert);
    shader.program.setUniformValue("dispalbonly",m_drawColors);
    shader.program.setUniformValue("disperror",m_drawError);
    shader.program.setUniformValue("dispshadow",m_drawShadows);
    shader.program.setUniformValue("dispcust",m_drawCustom);

    glEnable(GL_DEPTH_TEST);

    //glDepthMask(true);
    //glDepthFunc(GL_LEQUAL);
    //  glEnable(GL_CULL_FACE);


    //if(m_drawWireframe)
    //    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    //else
    //    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    QMap<QString,Mesh>::iterator it=meshes.begin();
    while(it!=meshes.end())
    {
        QMatrix4x4 modelViewMatrix = camera.view() * it.value().modelMatrix;
        QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
        QMatrix4x4 MVP = camera.projection() * modelViewMatrix;

        shader.program.setUniformValue("N",normalMatrix);
        shader.program.setUniformValue("MVP",MVP);


        it.value().draw(shader);
        it++;
    }


    // Draw black outline
    if(m_drawWireframe)
    {
        glPolygonOffset(-1.0f, -1.0f);      // Shift depth values
        glEnable(GL_POLYGON_OFFSET_LINE);

        // Draw lines antialiased
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw black wireframe version of geometry
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(12.5f);
        // draw wireframe on top of mesh
        //drawShadows();

        shader.program.setUniformValue("disptex",false);
        shader.program.setUniformValue("disptexnorm",false);
        shader.program.setUniformValue("dispnorm",false);
        shader.program.setUniformValue("dispalb",false);
        shader.program.setUniformValue("dispalbonly",false);
        shader.program.setUniformValue("disperror",false);
        shader.program.setUniformValue("dispshadow",true);
        shader.program.setUniformValue("dispcust",false);
        //glEnable(GL_DEPTH_TEST);
        //glDepthMask(false);

        //glPolygonMode(GL_BACK,GL_LINE);
        //glLineWidth(5);
        //glEnable( GL_POLYGON_OFFSET_LINE );
        //glPolygonOffset( -1, -1 );
        QMap<QString,Mesh>::iterator it2=meshes.begin();
        while(it2!=meshes.end())
        {
            QMatrix4x4 modelViewMatrix = camera.view() * it2.value().modelMatrix;
            QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
            QMatrix4x4 MVP = camera.projection() * modelViewMatrix;

            shader.program.setUniformValue("N",normalMatrix);
            shader.program.setUniformValue("MVP",MVP);


            it2.value().draw(shader);
            it2++;
        }
        //glDisable( GL_POLYGON_OFFSET_LINE );
        //drawLambert();
        //glDepthMask(true);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
    }
}


void GLWidget::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_L)
        m_controlLight = !m_controlLight;
    // rotation
    if(e->key() == Qt::Key_Right )
        camera.view().rotate(5, QVector3D(0,1,0));
    if(e->key() == Qt::Key_Left )
        camera.view().rotate(-5, QVector3D(0,1,0));
    if(e->key() == Qt::Key_Up )
        camera.view().rotate(5, QVector3D(1,0,0));
    if(e->key() == Qt::Key_Down )
        camera.view().rotate(-5, QVector3D(1,0,0));

}

void GLWidget::mouseMoveEvent(QMouseEvent *m)
{
    float width = this->width();
    float height = this->height();
    if(m->buttons() && m_controlLight)
    {
        QPointF pos = QPointF( m->pos().x() /width*2 -1, 1- m->pos().y()/height*2  );
        light = QVector3D (pos.x(),pos.y(),sqrt( qMax(1-pos.x()*pos.x()-pos.y()*pos.y(),0.0) ) );
    }

    float dx = (m->x() )/width*2-1;
    float dy = 1-(m->y() )/height*2;

    if (m->buttons() & Qt::LeftButton && !m_controlLight)
    {
        camera = Camera();
        camera.setZoom(lastFov);
        camera.view().translate(dx,dy,0);
    }
    else if (m->buttons() & Qt::RightButton && !m_controlLight)
    {
        if(!m_mouseLook)
        {
            lastPos = m->pos();
            m_mouseLook = true;
        }
        dx = (m->x() -lastPos.x());
        dy = (m->y() -lastPos.y());
        camera.view().rotate(dx/10.0,QVector3D(0,1,0));
        camera.view().rotate(dy/10.0,QVector3D(1,0,0));
        //camera.setZoom(lastFov);
        lastPos = m->pos();
    }
    else
    {
        m_mouseLook = false;
    }

}

void GLWidget::wheelEvent(QWheelEvent *w)
{
    float step = 2;
    float fov;
    if(w->delta() < 0)
        fov = qMin( lastFov + step, (float)120.0);
    else
        fov = qMax( lastFov - step, (float)10.0);
    camera.setZoom(fov);
    lastFov = fov;
}

void GLWidget::addMesh(QString meshName, Mesh mesh)
{
    makeCurrent();
    //Mesh newMesh=mesh;
    QMap<QString,Mesh>::iterator it=meshes.begin();
    while(it!=meshes.end())
    {
        it.value().clean();
        it++;
    }
    meshes.insert(meshName,mesh);
    meshes[meshName].setup();
}

// Private
void GLWidget::printContext()
{
    QString glType;
    QString glVersion;
    QString glProfile;

    // Get Version Information
    glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    // Get Profile Information
#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
    switch (format().profile())
    {
    CASE(NoProfile);
    CASE(CoreProfile);
    CASE(CompatibilityProfile);
    }
#undef CASE

    // qPrintable() will print our QString w/o quotes around it.
    qDebug() << glType +" "+ glVersion + "(" + glProfile + ")";
}

void GLWidget::controlLight()
{
    m_controlLight = !m_controlLight;
}


void GLWidget::drawTexture()
{
    m_drawTexture = true;
    m_drawCustom = false;
    m_drawTextureLambert = false;
    m_drawNormals = false;
    m_drawColors = false;
    m_drawLambert = false;
    m_drawShadows = false;
    m_drawError = false;
}

void GLWidget::drawTextureLambert()
{
    m_drawTextureLambert = true;
   m_drawCustom = false;
    m_drawTexture = false;
    m_drawNormals = false;
    m_drawColors = false;
    m_drawLambert = false;
    m_drawShadows = false;
    m_drawError = false;
}

void GLWidget::drawNormals()
{
    m_drawNormals = true;
    m_drawCustom = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawColors = false;
    m_drawLambert = false;
    m_drawShadows = false;
    m_drawError = false;
}

void GLWidget::drawColors()
{
    m_drawColors = true;
   m_drawCustom = false;
    m_drawNormals = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawLambert = false;
    m_drawShadows = false;
    m_drawError = false;
}

void GLWidget::drawLambert()
{
    m_drawLambert = true;
   m_drawCustom = false;
    m_drawColors = false;
    m_drawNormals = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawShadows = false;
    m_drawError = false;
}

void GLWidget::drawShadows()
{
    m_drawShadows = true;
   m_drawCustom = false;
    m_drawLambert = false;
    m_drawColors = false;
    m_drawNormals = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawError = false;
}

void GLWidget::drawError()
{
    m_drawError = true;
    m_drawCustom = false;
    m_drawLambert = false;
    m_drawColors = false;
    m_drawNormals = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawShadows = false;
}

void GLWidget::drawWireframe()
{
    m_drawWireframe = !m_drawWireframe;
}

void GLWidget::drawCustom()
{
    m_drawError = false;
    m_drawCustom = true;
    m_drawLambert = false;
    m_drawColors = false;
    m_drawNormals = false;
    m_drawTexture = false;
    m_drawTextureLambert = false;
    m_drawShadows = false;
}

void GLWidget::savePic()
{
    QString file = QFileDialog::getSaveFileName(this, "Save as...", "name", "PNG (*.png);; BMP (*.bmp);;TIFF (*.tiff *.tif);; JPEG (*.jpg *.jpeg)");
    //this->grabFramebuffer().save(file);

    QImage img = this->grabFramebuffer();
    qDebug() << "image grabbed! size "<<img.size();
    bool success = img.save(file);
    if(success)
        qDebug() << "image saved!";
    else
        qDebug() << "image NOT saved :(";
    //QPainter painter(&img);
    //this->render(&painter);
    //img.save("/home/mat/file.jpg");
}


QGroupBox* GLWidget::displayMenu()
{
   QGroupBox *m_menu = new QGroupBox("Display Options");

   QPushButton *wireBtm = new QPushButton("wireframe");
   connect(wireBtm,SIGNAL(pressed()),this,SLOT(drawWireframe()));

   QPushButton *texBtm = new QPushButton("texture");
   connect(texBtm,SIGNAL(pressed()),this,SLOT(drawTexture()));

   QPushButton *texnBtm = new QPushButton("texture+normals");
   connect(texnBtm,SIGNAL(pressed()),this,SLOT(drawTextureLambert()));

   QPushButton *colBtm = new QPushButton("albedo");
   connect(colBtm,SIGNAL(pressed()),this,SLOT(drawColors()));

   QPushButton *colnBtm = new QPushButton("albedo+normals");
   connect(colnBtm,SIGNAL(pressed()),this,SLOT(drawLambert()));

   QPushButton *normBtm = new QPushButton("normals");
   connect(normBtm,SIGNAL(pressed()),this,SLOT(drawNormals()));

   QPushButton *shadowBtm = new QPushButton("shadows");
   connect(shadowBtm,SIGNAL(pressed()),this,SLOT(drawShadows()));

   QPushButton *errorBtm = new QPushButton("p-errors");
   connect(errorBtm,SIGNAL(pressed()),this,SLOT(drawError()));

   QPushButton *customBtm = new QPushButton("red");
   connect(customBtm,SIGNAL(pressed()),this,SLOT(drawCustom()));

   QPushButton *lightBtm = new QPushButton("moveLight");
   connect(lightBtm,SIGNAL(pressed()),this,SLOT(controlLight()));

   QPushButton *savePicBtm = new QPushButton("savePic");
   connect(savePicBtm,SIGNAL(pressed()),this,SLOT(savePic()));

   QGridLayout *menuLayout = new QGridLayout;
   menuLayout->addWidget(wireBtm,0,0);
   menuLayout->addWidget(lightBtm,0,1);
   menuLayout->addWidget(texBtm,1,0);
   menuLayout->addWidget(texnBtm,1,1);
   menuLayout->addWidget(colBtm,2,0);
   menuLayout->addWidget(colnBtm,2,1);
   menuLayout->addWidget(normBtm,3,0);
   menuLayout->addWidget(shadowBtm,3,1);
   menuLayout->addWidget(errorBtm,4,0);
   menuLayout->addWidget(savePicBtm,4,1);
   menuLayout->addWidget(customBtm,5,0);

   m_menu->setLayout(menuLayout);
   m_menu->setSizePolicy(QSizePolicy( QSizePolicy::Fixed,QSizePolicy::Fixed));
   return m_menu;
}

