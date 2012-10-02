import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

//String filename = "../../../../Dev/avrgcc/Networked-Display/FrameServer/pixelbuffer.bin";
//String filename = "D:/Dev/avrgcc/Networked-Display/FrameServer/pixelbuffer.bin";
//String filename = "/Users/Louis/Dropbox/Dev/avrgcc/Networked-Display/FrameServer/pixelbuffer.bin";
String filename = "C:/pixelbuffer.bin";
String keypressfilename = "C:/vncinput.bin";

void saveFrameBuffer ()
{
    try
    {
        RandomAccessFile
        rac = new
        RandomAccessFile(filename, "rw");
        FileChannel channel = rac.getChannel();
        MappedByteBuffer buf = channel.map(MapMode.READ_WRITE, 0,
                128 * 160 * 4);
        
        byte[] frameBuffer = new byte[160*128*4];

        loadPixels();
        for (int i = 0; i < 128 * 160; i++)
        {
            frameBuffer[ (i * 4) + 0 ] = (byte)red(pixels[ i ]);
            frameBuffer[ (i * 4) + 1 ] = (byte)green(pixels[ i ]);
            frameBuffer[ (i * 4) + 2 ] = (byte)blue(pixels[ i ]);
        }
        buf.put(frameBuffer);
        updatePixels();
    }
    
    catch (IOException e)
    {   
        e.printStackTrace();
    }
}
/**
 * Moving On Curves. 
 * 
 * In this example, the circles moves along the curve y = x^4.
 * Click the mouse to have it move to a new position.
 */

float beginX = 20.0;  // Initial x-coordinate
float beginY = 10.0;  // Initial y-coordinate
float endX = 110.0;   // Final x-coordinate
float endY = 150.0;   // Final y-coordinate
float distX;          // X-axis distance to move
float distY;          // Y-axis distance to move
float exponent = 4;   // Determines the curve
float x = 0.0;        // Current x-coordinate
float y = 0.0;        // Current y-coordinate
float step = 0.03;    // Size of each step along the path
float pct = 0.0;      // Percentage traveled (0.0 to 1.0)

void setup() 
{
  size(128, 160);
  //pad = loadImage("background.jpg");
  background(loadImage("background.jpg"));
  noStroke();
  smooth();
  distX = endX - beginX;
  distY = endY - beginY;
  
  rectMode(RADIUS); 
}

void keyPressed() {
  interpretKeypress(key);
}

void checkKeypressFile() {
    try
    {
      byte[] tempbuffer = new byte[1];
      RandomAccessFile rac = new RandomAccessFile(keypressfilename, "rw");
      FileChannel channel = rac.getChannel();
      MappedByteBuffer buf = channel.map(MapMode.READ_WRITE, 0,1);

      buf.get(tempbuffer);

      if(tempbuffer[0] >= 'a' && tempbuffer[0] <= 'z')
      {
        print(tempbuffer[0]);
        interpretKeypress((int)tempbuffer[0]);
      }
      buf.put(0,(byte)0);
      rac.close();
    }
    
    catch (IOException e)
    {   
        e.printStackTrace();
    }  
}

int shapeWidth = 20;
int shapeHeight = 20;
int shapeType = 0;

void interpretKeypress(int key)
{
    if(key == 'a')
    {
      if(shapeWidth < 40)
        shapeWidth += 5;
      if(shapeHeight < 40)
        shapeHeight += 5;
    }
    if(key == 'b')
    {
      if(shapeWidth > 10)
        shapeWidth -= 5;  
      if(shapeHeight > 10)
        shapeHeight -= 5;
    }
    if(key == 'c')
    {
      if(shapeType == 0)
        shapeType = 1;
      else shapeType = 0;
    }  
    if(key == 'r')
    {
      shapeWidth = 20;
      shapeHeight = 20;
      shapeType = 0;
      background(loadImage("background.jpg"));
    }
}

color fillColor = color(random(255), random(255), random(255), 255);

void draw() 
{
  fill(0, 2);
  pct += step;
  if (pct < 1.0) {
    x = beginX + (pct * distX);
    y = beginY + (pow(pct, exponent) * distY);
  }
  else
  {
    checkKeypressFile();

    pct = 0.0;
    beginX = x;
    beginY = y;
    endX = random(width);
    endY = random(height);
    distX = endX - beginX;
    distY = endY - beginY;
  }

  fillColor = color(random(255), random(255), random(255), 255);

  fill(fillColor);
  if(shapeType == 0)
    ellipse(x, y, shapeWidth, shapeHeight);
  else
      rect(x, y, shapeWidth/2, shapeHeight/2);

  saveFrameBuffer();
}

void mousePressed() {
  pct = 0.0;
  beginX = x;
  beginY = y;
  endX = mouseX;
  endY = mouseY;
  distX = endX - beginX;
  distY = endY - beginY;
}
