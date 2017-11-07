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
//12 Laptop and Fan
//13 Monitor and Lamp



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
PImage greenback;
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

PFont fontA;

int border = 30;
int small_border = 5;
int[] out = new int[2];

boolean mouseclicked = false;

void setup()
{

  //Initialize Serial Port,  "COMX" depends on the port number
  //38400 transfer speed
  port = new Serial(this, "COM5", 38400);

  greenback = loadImage("greendisplay.jpg");
  //background border
  size(1370, 770);
  background(calplugblue);
 fill(calpluggreen);
  rect(border, border, width-2*border, height-2*border);

  //title of template: background
  stroke(white);
  fill(white);
  rect(80+small_border, 70+small_border, width-160-2*small_border, 100-2*small_border);

  //Loads a font
  fontA = loadFont("SansSerif.bold-48.vlw");
  textFont(fontA, 40);


  //Appliance Box: background
  stroke(white);
  fill(white);
  rect(80+small_border, 240+small_border, width/2-120-2*small_border, 450-2*small_border);


  //Image Box: background
  stroke(white);
  fill(white);
  rect(width/2+80+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);


  // Title font
  fill(black);
  text("ENERGY MANAGEMENT SYSTEM", 100, 140);

  // Appliances Text
  textFont(fontA, 60);
  img = loadImage("calplug_logo.png");
  lamp = loadImage("lamp.jpg");
  hp = loadImage("HP_Laptop.jpg");
  fan = loadImage("Vornado_Foot_Fan.jpg");
  tv = loadImage("Sharp_TV.jpg");
  stb = loadImage("Set_Top_Box.jpg");
  fridge = loadImage("FRIDGE.jpg");
  question = loadImage("Question_mark.PNG");
  toaster = loadImage("toaster.jpg");
  monitor = loadImage("acer.jpg");
  printer = loadImage("printer.jpg");

  image(img, 950, 90);

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
  //out[0] = 1;
  println(temp);
  println(out[0]);
  println(out[1]);

  if (out[0] == 0)
  {
    erase_all();
    fill(red);
    image(question, width/2+200, 350);
    text("Unknown", 220, 450);
    text("Device", 250, 520);
  }
  

  if (out[0] == 1)
  {
    erase_all();
    fill(blue);
    text("Fan",120, 400);
    image(fan, width/2+200, 350);
    text("Quantity: 1", 120, 500);
    
    textSize(30);
    text("Power Comsumption: 147W", 120,600);
    textSize(60);
  }

  if (out[0] == 2)
  {    
    erase_all();
    fill(blue);
    text("Fan",120, 400);
    image(fan, width/2+200, 350);
    text("Quantity: 2", 120, 500);
    
    textSize(30);
    text("Power Comsumption: 286W", 120,600);
    textSize(60);
  }
  
  if (out[0] == 3)
  {        
    erase_all();
    image(hp, width/2+200, 350);
    fill(calplugblue);
    text("Quantity: 1", 200, 550);  
    text("HP Laptop", 200, 450);
    
    textSize(30);
    text("Power Comsumption: 69.4W", 120,600);
    textSize(60);
  }

  if (out[0] == 4)
  { 
    erase_all();
    fill(calplugblue);
    image(hp, width/2+200, 350);
    text("HP Laptop", 200, 450);
    text("Quantity: 2", 200, 550);
  }

  if (out[0] == 5)
  {
    erase_all();
    fill(calplugblue);
    image(lamp, width/2+120, 270, width/3, height/2);
    text("Desk Lamp", 210, 450);
    text("Quantity: 1", 210, 520);
    
    textSize(30);
    text("Power Comsumption: 9.1W", 120,600);
    textSize(60);
  }

  if (out[0] == 6)
  {
    erase_all();
    fill(blue);
    image(fridge, width/2+200, 350);
    text("Refrigerator", 220, 450);
    text("Quantity: 1", 250, 520);
  }
  if (out[0] == 7)
  {
    erase_all();
    fill(calplugblue);
    image(toaster, width/2+200, 350);
    text("Toaster", 200, 430);
    text("Quantity: 1", 200, 500);
    
    textSize(30);
    text("Power Comsumption: 684W", 120,600);
    textSize(60);
  }
  if (out[0] == 8)
  {
    erase_all();
    fill(calplugblue);
    image(monitor, width/2+120, 350, width/3, height/3);
    text("Monitor", 200, 430);
    text("Quantity: 1", 200, 500);
    
    textSize(30);
    text("Power Comsumption: 15.7W", 120,600);
    textSize(60);
  }
  if (out[0] == 9)
  {
    erase_all();
    fill(calplugblue);
    image(printer, width/2+120, 270, width/3, height/2);
    text("Printer", 200, 430);
    text("Quantity: 1", 200, 500);
  }
  if (out[0] == 10)
  {
    erase_all();
    fill(calplugblue);
    image(fridge, width/2+140, 300, width/6, height/5);
    image(toaster, width/2+300, 500, width/6, height/5);
    text("Toaster &", 150, 430);
    text("Fridge", 150, 500);
  }
  if (out[0] == 11)
  {
    erase_all();
    fill(calplugblue);
    image(monitor, width/2+140, 270);
    image(hp,width/2+300,470);
    text("Monitor &", 150, 430);
    text("Laptop", 150, 500);
  }
  if (out[0] == 12)
  {
    erase_all();
    fill(calplugblue);
    image(hp, width/2+110, 250);
    image(fan, width/2+300,450);
    text("Laptop &", 230, 430);
    text("Fan", 250, 500);
    
    textSize(30);
    text("Power Comsumption: 214W", 120,600);
    textSize(60);
  }
  if (out[0] == 13)
  {
    erase_all();
    fill(calplugblue);
    image(monitor, width/2+110, 250);
    image(lamp, width/2+350,500,width/6, height/5);
    text("Monitor &",230, 430);
    text("Lamp", 250, 500);
    
    textSize(30);
    text("Power Comsumption: 24W", 120,600);
    textSize(60);
  }
  loop();
}

void erase_all()
{
  //erase all previous letters
  fill(white);
  text("Quantity: 2", (width/2+200), 650);
  text("Quantity: 1", (width/2+200), 650);

  //erase previous images
  stroke(white);
  fill(white);
  rect(width/2+80+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);
  //rect(width/2+200+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);
  rect(80+small_border, 240+small_border, width/2-120-2*small_border, 450-2*small_border);
}