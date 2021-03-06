#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"

#include "text/FontStore.h"
#include "text/TextBox.h"

using namespace ci;
using namespace ci::app;
using namespace ph::text;
using namespace std;

class TextRenderingApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();
	void shutdown();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	

	void keyDown( KeyEvent event );

	void resize( ResizeEvent event );
	void fileDrop( FileDropEvent event );
protected:
	void updateWindowTitle();
protected:
	bool			mShowBounds;
	bool			mShowWireframe;

	Color			mFrontColor;
	Color			mBackColor;

	//! 
	TextBox			mTextBox;
	//!
	gl::GlslProg	mSdfShader;
};

void TextRenderingApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate(200.0f);
	settings->setWindowSize(640, 720);
}

void TextRenderingApp::setup()
{
	mShowBounds = false;
	mShowWireframe= false;

	mFrontColor = Color::black();
	mBackColor = Color::white();

	try { 
		// load fonts using the FontStore
		fonts().loadFont( loadAsset("fonts/BubblegumSans-Regular.sdff") ); 

		// create a text box (rectangular text area)
		mTextBox = TextBox( getWindowSize() );
		// set font and font size
		mTextBox.setFont( fonts().getFont("BubblegumSans-Regular") );
		mTextBox.setFontSize( 24.0f );
		// break lines between words
		mTextBox.setBoundary( Text::WORD );
		// adjust space between lines
		mTextBox.setLineSpace( 1.0f );

		// load a long text and hand it to the text box
		mTextBox.setText( loadString( loadAsset("fonts/readme.txt") ) );

		// load and compile the Signed Distance Field shader
		mSdfShader = gl::GlslProg( loadAsset("shaders/font_sdf_vert.glsl"), loadAsset("shaders/font_sdf_frag.glsl") );
	}
	catch( const std::exception & e ) { 
		console() << e.what() << std::endl; 
	}

	updateWindowTitle();
}

void TextRenderingApp::shutdown()
{
}

void TextRenderingApp::update()
{
}

void TextRenderingApp::draw()
{
	// note: when drawing text to an FBO, 
	//  make sure to enable maximum quality multisampling (16x is best)!

	// clear out the window
	gl::clear( mBackColor );

	// draw text box centered inside the window
	Area fit = Area::proportionalFit( Area(Vec2i::zero(), mTextBox.getSize()), getWindowBounds(), true, false ); 

	gl::color( mFrontColor );
	gl::enableAlphaBlending();

	gl::pushModelView();
	gl::translate( fit.getUL() );

	// bind shader
	if(mSdfShader) {
		mSdfShader.bind();
		mSdfShader.uniform("tex0", 0);
	}

	mTextBox.draw();

	// unbind shader
	if(mSdfShader) mSdfShader.unbind();

	if(mShowWireframe) mTextBox.drawWireframe();
	
	gl::popModelView();
	gl::disableAlphaBlending();

	if(mShowBounds)
		mTextBox.drawBounds( fit.getUL() );
}

void TextRenderingApp::mouseDown( MouseEvent event )
{
}

void TextRenderingApp::keyDown( KeyEvent event )
{
	switch(event.getCode()) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_w:
		mShowWireframe = !mShowWireframe;
		break;
	case KeyEvent::KEY_b:
		mShowBounds = !mShowBounds;
		break;
	case KeyEvent::KEY_SPACE:
		{ Color temp = mFrontColor; mFrontColor = mBackColor; mBackColor = temp; }
		break;
	case KeyEvent::KEY_LEFTBRACKET:
		if(mTextBox.getFontSize() > 0.5f)
			mTextBox.setFontSize( mTextBox.getFontSize() - 0.5f );
		break;
	case KeyEvent::KEY_RIGHTBRACKET:
		mTextBox.setFontSize( mTextBox.getFontSize() + 0.5f );
		break;
	case KeyEvent::KEY_LEFT:
		if( mTextBox.getAlignment() == TextBox::RIGHT )
			mTextBox.setAlignment( TextBox::CENTER );
		else if( mTextBox.getAlignment() == TextBox::CENTER )
			mTextBox.setAlignment( TextBox::LEFT );
		break;
	case KeyEvent::KEY_RIGHT:
		if( mTextBox.getAlignment() == TextBox::LEFT )
			mTextBox.setAlignment( TextBox::CENTER );
		else if( mTextBox.getAlignment() == TextBox::CENTER )
			mTextBox.setAlignment( TextBox::RIGHT );
		break;
	case KeyEvent::KEY_UP:
		if( mTextBox.getLineSpace() > 0.2f )
			mTextBox.setLineSpace( mTextBox.getLineSpace() - 0.1f );
		break;
	case KeyEvent::KEY_DOWN:
		if( mTextBox.getLineSpace() < 5.0f )
			mTextBox.setLineSpace( mTextBox.getLineSpace() + 0.1f );
		break;
	}

	//
	updateWindowTitle();
}

void TextRenderingApp::resize( ResizeEvent event )
{
	// allow 20 pixels margin
	Area box( Vec2i::zero(), event.getSize() );
	box.expand(-20, -20);

	mTextBox.setSize( box.getSize() );
}

void TextRenderingApp::fileDrop( FileDropEvent event )
{
	if( event.getNumFiles() == 1) {
		// you dropped 1 file, let's try to load it as a SDFF font file
		fs::path file = event.getFile(0);

		if(file.extension() == ".sdff") {
			try {
				// create a new font and read the file
				ph::text::FontRef font( new ph::text::Font() );
				font->read( loadFile(file) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
		else {
			// try to render the file as a text
			mTextBox.setText( loadString( loadFile(file) ) );
		}
	}
	else if( event.getNumFiles() == 2 ) {
		// you dropped 2 files, let's try to create a new SDFF font file
		fs::path fileA = event.getFile(0);
		fs::path fileB = event.getFile(1);

		if(fileA.extension() == ".txt" && fileB.extension() == ".png") {
			try {
				// create a new font from an image and a metrics file
				ph::text::FontRef font( new ph::text::Font() );
				font->create( loadFile(fileB), loadFile(fileA) );
				// create a compact SDFF file
				font->write( writeFile( getAssetPath("") / "fonts" / (font->getFamily() + ".sdff") ) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
		else if(fileB.extension() == ".txt" && fileA.extension() == ".png") {
			try {
				// create a new font from an image and a metrics file
				ph::text::FontRef font( new ph::text::Font() );
				font->create( loadFile(fileA), loadFile(fileB) );
				// create a compact SDFF file
				font->write( writeFile( getAssetPath("") / "fonts" / (font->getFamily() + ".sdff") ) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
	}

	updateWindowTitle();
}

void TextRenderingApp::updateWindowTitle()
{
#ifdef WIN32
	std::wstringstream str;
	str << "TextRenderingApp -";
	str << " Font family: " << toUtf16( mTextBox.getFontFamily() );
	str << " (" << mTextBox.getFontSize() << ")";

	HWND hWnd = getRenderer()->getHwnd();
	::SetWindowText( hWnd, str.str().c_str() );
#endif
}

CINDER_APP_BASIC( TextRenderingApp, RendererGl )
