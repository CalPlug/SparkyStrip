//0 Unrecognizable
//1 fan Quantity: 1
//2 Fan Q: 2
//3 Hp Laptop Q: 1
//4 Hp Laptop Q: 2
//5 Lamp Q: 1
//6 Fridge
//7 Toaster
//8 Monitor
//9 Printer
//10 Fridge and Toaster
//11 Monitor and Laptop
//12 



import processing.serial.*;
Serial port;  //  Serial port name

color light_green = color(128, 255, 0);
color black = color(0, 0, 0);
color white = color(255, 255, 255);
color blue = color(0, 89, 255);
color red = color(255, 0, 0);
color realblue = color(0, 0, 255);
color green = color(0, 255, 0);
color yellow = color(255, 255, 0);
color cyan = color(255, 0, 255);
color calplugblue = color(36, 76, 180);
color calpluggreen =color(131, 216, 42);

// Declaring a variable of type PImage
PImage img;
PImage hp;
PImage fan;
PImage tv;
PImage stb;
PImage question;
PImage lamp;
PImage fridge;
PImage toaster;
PImage monitor;
PImage printer;


//load string
//String[] output;
String output;

PFont fontA, fontB;

int border = 30;
int small_border = 5;
int[] out = new int[2];

boolean mouseclicked = false;

void setup()
{

  //Initialize Serial Port,  "COMX" depends on the port number
  //38400 transfer speed
  port = new Serial(this, "COM9", 38400);

     size(1366,753);
     PImage circuitart;
     circuitart = loadImage("circuitboardart.jpg");
     background(circuitart);

  //background
  //stroke(black);
  //strokeWeight(4);
  //fill(calpluggreen);
  //rect(border, border, width-2*border, height-2*border);

  //title of template: border
  //fill(blue);
  //rect(80, 70, width-160, 100);

  //title of template: background
  //stroke(white);
  //fill(white);
  //rect(80+small_border, 70+small_border, width-160-2*small_border, 100-2*small_border);

  //Loads a font
  fontA = loadFont("ARDestine-48.vlw");
  fontB = loadFont("OCRAExtended-48.vlw");
  textFont(fontA, 40);

  // Title font
  fill(calpluggreen);
  text("ENERGY MANAGEMENT SYSTEM", width/1.8, height/5);

  // Appliances Text
  
  textFont(fontB, 60);
  //text("Vornado Fan", 100, 300);
  //text("", 100, 390);
  //text("Light Bulb", 200, 480);
  //text("HP Laptop", 100, 570);
  //text("Unidentified Appliance", 100, 660);
  //text("The Device is a:", width/2+100, 300);

  /*/ ellipses, indicating whether or not program "detects" item
   for (int i = 0; i < 5; i = i+1)
   {
   stroke(black);
   fill(white);
   ellipse(width/2-80, 285+i*90, 20, 20);
   }
   */

  img = loadImage("calpluglogo.jpg");
  lamp = loadImage("lamp.jpg");
  hp = loadImage("HP_Laptop.jpg");
  fan = loadImage("Vornado_Foot_Fan.jpg");
  tv = loadImage("Sharp_TV.jpg");
  stb = loadImage("Set_Top_Box.jpg");
  fridge = loadImage("FRIDGE.jpg");
  question = loadImage("questionmark1.png");
  toaster = loadImage("toaster.jpg");
  monitor = loadImage("acer.jpg");
  printer = loadImage("printer.jpg");

  image(img, width/1.375, height/20);

  noLoop();
}

String temp;

void draw()
{


  byte[] inBuffer = new byte[2];
  inBuffer = port.readBytes();


  if (inBuffer != null)
  {
    for (int j =0; j < 2; j++)
    {
      out[j] = int(inBuffer[0]);
      //temp = str(inBuffer[0]);
    }
  }

  println(temp);
  println(out[0]);
  println(out[1]);
 
 
 
 //INITIALIZE OUT FOR TESTING
 out[0] =2 ;
 //--------------------------- 
  
  
  if (out[0] == 0)
  {
    erase_all();
    fill(red);
    image(question, width/3+130, 300);
    textFont(fontB, 80);
    text("Unknown", width/5.6, 440);
    text("Device", width/1.7, 440);
  }
 

  if (out[0] == 1)
  {
    erase_all();
    image(fan, width/2-100, 300);
    fill(red);
    //text("Quantity: 1", (width/2+200), 650);     
    textFont(fontB, 80);
    text("Vornado Foot Warmer", 430, 450);
  }

  if (out[0] == 2)
  {    
    erase_all();
    fill(red);
    textFont(fontB, 60);
    text("Vornado \nFoot Warmer",150, 450);
    image(fan, width/2-100, 300);
    text("Quantity:2", 530, 620);
  }
  if (out[0] == 3)
  {        
    erase_all();
    image(hp, width/2-100, 300);
    fill(red);
    textFont(fontB,80);
    //text("Quantity: 1", (width/2+200), 650);     
    text("Laptop", 240, 450);
  }

  if (out[0] == 4)
  { 
    erase_all();
    fill(red);
    image(hp, width/2-300, 170);
    image(hp, width/2+70, 400);
    text("HP Laptop", 750, 300);
    text("Quantity: 2", 270, 550);
  }

  if (out[0] == 5)
  {
    erase_all();
    fill(red);
    textFont(fontB,80);
    image(lamp, width/2-160, 270, width/4, height/3);
    text("Desk Lamp", 80, 430);
    //text("Quantity: 1", 210, 520);
  }

  if (out[0] == 6)
  {
    erase_all();
    fill(red);
    textFont(fontB, 70);
    image(fridge, width/2-100, 320);
    text("Refrigerator", 70, 440);
    //text("Quantity: 1", 250, 520);
  }
  if (out[0] == 7)
  {
    erase_all();
    fill(red);
    textFont(fontB,80);
    image(toaster, width/2-100, 270);
    text("Toaster", 200, 430);
    //text("Quantity: 1", 200, 500);
  }
  if (out[0] == 8)
  {
    erase_all();
    fill(red);
    textFont(fontB,80);
    image(monitor, width/2-130, 300, width/3, height/3);
    text("Monitor", 190, 430);
    //text("Quantity: 1", 200, 500);
  }
  if (out[0] == 9)
  {
    erase_all();
    fill(red);
    textFont(fontB, 80);
    image(printer, width/2-140, 250, width/3, height/2);
    text("Printer", 180, 430);
    //text("Quantity: 1", 200, 500);
  }
  if (out[0] == 10)
  {
    erase_all();
    fill(red);
    textFont(fontB,80);
    image(fridge, width/2-180, 270, width/6, height/5);
    image(toaster, width/2+60, 480, width/6, height/5);
    text("Toaster", 330, 560);
    text("Fridge", 770, 350);
  }
  if (out[0] == 11)
  {
    erase_all();
    fill(red);
    textFont(fontB, 80);
    image(monitor, width/2-180, 240);
    image(hp,width/2+60,470);
    text("Monitor", 780, 390);
    text("Laptop", 440, 580);
  }
  if (out[0] == 12)
  {
    erase_all();
    fill(red);
    textFont(fontB, 80);
    image(hp, width/2-300, 210);
    image(fan, width/2+50,450);
    text("Laptop", 750, 340);
    text("Fan", 550, 560);
  }
  if (out[0] == 13)
  {
    erase_all();
    fill(red);
    textFont(fontB, 80);
    image(hp, width/2-250, 230);
    image(lamp, width/2+50,480,width/5, height/4);
    text("Laptop",760, 360);
    text("Lamp", 530, 580);
  }
  loop();
}
//  text("Rise Time: " + output[1], width/2+20, 600);
//  text("Normalized Magnitude at 180 Hz: " + output[2], width/2+20, 660);
//  text("Normalized Magnitude at 300 Hz: " + output[3], width/2+20, 720);




void erase_all()
{
   size(1366,753);
   PImage circuitart;
   circuitart = loadImage("circuitboardart.jpg");
   background(circuitart);
  
  textFont(fontA, 40); 
  fill(calpluggreen);
  text("ENERGY MANAGEMENT SYSTEM", width/1.8, height/5);
  
  PImage img;
  img = loadImage("calpluglogo.jpg");
  image(img, width/1.375, height/20);

  //erase all previous letters
  //fill(white);
  //text("Quantity: 2", (width/2+200), 650);
  //text("Quantity: 1", (width/2+200), 650);

  //erase all previous ellipses
  /*ellipse(width/2-80, 645, 15, 15);
   ellipse(width/2-80, 285, 15, 15);
   ellipse(width/2-80, 555, 15, 15);
   ellipse(width/2-80, 465, 15, 15); 
   */

  //erase previous images
 // stroke(white);
  //fill(white);
  //rect(width/2+80+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);
  //rect(width/2+200+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);
  //rect(80+small_border, 240+small_border, width/2-120-2*small_border, 450-2*small_border);
}


