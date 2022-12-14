/*
  CSC C85 - Fundamentals of Robotics and Automated Systems

  Particle filters implementation for a simple robot.

  Your goal here is to implement a simple particle filter
  algorithm for robot localization.

  A map file in .ppm format is read from disk, the map
  contains empty spaces and walls.

  A simple robot is randomly placed on this map, and
  your task is to write a particle filter algorithm to
  allow the robot to determine its location with
  high certainty.

  You must complete all sections marked

  TO DO:

  NOTE: 2 keyboard controls are provided:

  q -> quit (exit program)
  r -> reset particle set during simulation

  Written by F.J.E. for CSC C85, May 2012. U
  pdated Oct. 2021

  This robot model inspired by Sebastian Thrun's
  model in CS373.
*/

#include "ParticleFilters.h"
#include "prob.h"

/**********************************************************
 GLOBAL DATA
**********************************************************/
unsigned char *map;     // Input map
unsigned char *map_b;   // Temporary frame
struct particle *robot; // Robot
struct particle *list;  // Particle list
int sx, sy;             // Size of the map image
char name[1024];        // Name of the map
int n_particles;        // Number of particles
int windowID;           // Glut window ID (for display)
int Win[2];             // window (x,y) size
int RESETflag;          // RESET particles

/**********************************************************
 PROGRAM CODE
**********************************************************/
static int compare (const void * a, const void * b)
{
  if (*(double*)a > *(double*)b) return 1;
  else if (*(double*)a < *(double*)b) return -1;
  else return 0;
}

int main(int argc, char *argv[])
{
  /*
    Main function. Usage for this program:

    ParticleFilters map_name n_particles

    Where:
     map_name is the name of a .ppm file containing the map. The map
              should be BLACK on empty (free) space, and coloured
              wherever there are obstacles or walls. Anythin not
              black is an obstacle.

     n_particles is the number of particles to simulate in [100, 50000]

    Main loads the map image, initializes a robot at a random location
     in the map, and sets up the OpenGL stuff before entering the
     filtering loop.
  */

  if (argc != 3)
  {
    fprintf(stderr, "Wrong number of parameters. Usage: ParticleFilters map_name n_particles.\n");
    exit(0);
  }

  strcpy(&name[0], argv[1]);
  n_particles = atoi(argv[2]);

  if (n_particles < 100 || n_particles > 50000)
  {
    fprintf(stderr, "Number of particles must be in [100, 50000]\n");
    exit(0);
  }

  fprintf(stderr, "Reading input map\n");
  map = readPPMimage(name, &sx, &sy);
  if (map == NULL)
  {
    fprintf(stderr, "Unable to open input map, or not a .ppm file\n");
    exit(0);
  }

  // Allocate memory for the temporary frame
  fprintf(stderr, "Allocating temp. frame\n");
  map_b = (unsigned char *)calloc(sx * sy * 3, sizeof(unsigned char));
  if (map_b == NULL)
  {
    fprintf(stderr, "Out of memory allocating image data\n");
    free(map);
    exit(0);
  }

  srand48((long)time(NULL)); // Initialize random generator from timer
  // CHANGE the line above to 'srand48(12345);'  to get a consistent sequence of random numbers for testing and debugging your code!

  // INITIALIZE the robot at a random location and orientation.
  fprintf(stderr, "Init robot...\n");
  robot = initRobot(map, sx, sy);
  if (robot == NULL)
  {
    fprintf(stderr, "Unable to initialize robot.\n");
    free(map);
    free(map_b);
    exit(0);
  }
  sonar_measurement(robot, map, sx, sy); // Initial measurements...

  // Initialize particles at random locations
  fprintf(stderr, "Init particles...\n");
  list = NULL;
  initParticles();

  // Done, set up OpenGL and call particle filter loop
  fprintf(stderr, "Entering main loop...\n");
  Win[0] = 800;
  Win[1] = 800;
  glutInit(&argc, argv);
  initGlut(argv[0]);
  glutMainLoop();

  // This point is NEVER reached... memory leaks must be resolved by OpenGL main loop
  exit(0);
}

void initParticles(void)
{
  /*
    This function creates and returns a linked list of particles
    initialized with random locations (not over obstacles or walls)
    and random orientations.

    There is a utility function to help you find whether a particle
    is on top of a wall.

    Use the global pointer 'list' to keep trak of the *HEAD* of the
    linked list.

    Probabilities should be uniform for the initial set.
  */

  list = NULL;

  /***************************************************************
  // TO DO: Complete this function to generate an initially random
  //        list of particles. (DONE)
  ***************************************************************/

  // Loop n_particles times
  for (int i = 0; i < n_particles; i++)
  {
    // Create new particle w/ randomly initialized position
    struct particle *newParticle = initRobot(map, sx, sy);

    // Inititialize probability to be uniform for all particles but summing to 1 across all particles
    newParticle->prob = 1 / (double)n_particles; // Must cast the numerator or denominator to double to prevent integer division

    // Set "next" as "list", which is the current head
    newParticle->next = list;

    // Set new particle as the head
    list = newParticle;
  }
}

void computeLikelihood(struct particle *p, struct particle *rob, double noise_sigma)
{
  /*
    This function computes the likelihood of a particle p given the sensor
    readings from robot 'robot'.

    Both particles and robot have a measurements array 'measureD' with 16
    entries corresponding to 16 sonar slices.

    The likelihood of a particle depends on how closely its measureD
    values match the values the robot is 'observing' around it. Of
    course, the measureD values for a particle come from the input
    map directly, and are not produced by a simulated sonar.

    To be clear: The robot uses the sonar_measurement() function to
     get readings of distance to walls from its current location

    The particles use the ground_truth() function to determine what
     distance the walls actually are from the location of the particle
     given the map.

    You then have to compare what the particles should be measuring
     with what the robot measured, and determine the appropriate
     probability of the difference between the robot measurements
     and what the particles expect to see.

    Assume each sonar-slice measurement is independent, and if

    error_i = (p->measureD[i])-(sonar->measureD[i])

    is the error for slice i, the probability of observing such an
    error is given by a Gaussian distribution with sigma=20 (the
    noise standard deviation hardcoded into the robot's sonar).

    You may want to check your numbers are not all going to zero...
    (this can happen easily when you multiply many small numbers
     together).

    This function updates the Belief B(p_i) for the particle
    stored in 'p->prob'
  */

  /****************************************************************
  // TO DO: Complete this function to calculate the particle's
  //        likelihood given the robot's measurements (DONE)
  ****************************************************************/

  double sum_squared = 0;
  for (int i = 0; i < 16; i++)
  {
    // Calculate error for each slice
    double error = (p->measureD[i]) - (rob->measureD[i]);
    sum_squared += pow(error, 2);
  }

  // Divide sum of probabilities by number of sonar slices (16) and assign average probability to particle
  //p->prob = sumProb / 16;
  p->prob = 1/sum_squared;
}

void ParticleFilterLoop(void)
{
  /*
     Main loop of the particle filter
  */

  // OpenGL variables. Do not remove
  unsigned char *tmp;
  GLuint texture;
  static int first_frame = 1;
  double max;
  struct particle *p, *pmax;
  char line[1024];
  // Add any local variables you need right below.

  if (!first_frame)
  {
    // Step 1 - Move all particles a given distance forward (this will be in
    //          whatever direction the particle is currently looking).
    //          To avoid at this point the problem of 'navigating' the
    //          environment, any particle whose motion would result
    //          in hitting a wall should be bounced off into a random
    //          direction.
    //          Once the particle has been moved, we call ground_truth(p)
    //          for the particle. This tells us what we would
    //          expect to sense if the robot were actually at the particle's
    //          location.
    //
    //          Don't forget to move the robot the same distance!

    /******************************************************************
    // TO DO: Complete Step 1 and test it
    //        You should see a moving robot and sonar figure with
    //        a set of moving particles.
    ******************************************************************/
    // move all particles (and the robot at the end)
    double dist = 1;
    struct particle* p = list;
    for (int i = 0; i < n_particles+1; i++)
    {
      double curr_x = p->x;
      double curr_y = p->y;
      double curr_theta = p->theta;
      move(p, dist);
      // check if hit an obstacle
      int threshold = 20;
      int count = 0;
      while (hit(p, map, sx, sy) && count < threshold)
      {
        // revert position and randomize direction
        p->x = curr_x;
        p->y = curr_y;
        p->theta = drand48()*360; // NEED TO RANDOMIZE
        move(p, dist);
        count++;
      }
      // find ground truth (update sensor reading)
      ground_truth(p, map, sx, sy);
      // go to next particle if not last particle
      if (p->next != NULL)
      {
        p = p->next;
      }
      else if (p != robot)
      {
        p = robot;
      }
    }
    
    // Step 2 - The robot makes a measurement - use the sonar
    sonar_measurement(robot, map, sx, sy);

    // Step 3 - Compute the likelihood for particles based on the sensor
    //          measurement. See 'computeLikelihood()' and call it for
    //          each particle. Once you have a likelihood for every
    //          particle, turn it into a probability by ensuring that
    //          the sum of the likelihoods for all particles is 1.

    /*******************************************************************
    // TO DO: Complete Step 3 and test it
    //        You should see the brightness of particles change
    //        depending on how well they agree with the robot's
    //        sonar measurements. If all goes well, particles
    //        that agree with the robot's position/direction
    //        should be brightest.
    *******************************************************************/
    // Compute likelihood for all particles
    p = list;
    double total_probability = 0;
    for (int i = 0; i < n_particles; i++)
    {
      computeLikelihood(p, robot, 20);
      total_probability += p->prob;
      if (p->next != NULL)
      {
        p = p->next;
      }
    }
    // reweight probabilities so that they sum to 1;
    p = list;
    for (int i = 0; i < n_particles; i++)
    {
      p->prob = p->prob / total_probability;
      if (p->next != NULL)
      {
        p = p->next;
      }
    }

    // Step 4 - Resample particle set based on the probabilities. The goal
    //          of this is to obtain a particle set that better reflect our
    //          current belief on the location and direction of motion
    //          for the robot. Particles that have higher probability will
    //          be selected more often than those with lower probability.
    //
    //          To do this: Create a separate (new) list of particles,
    //                      for each of 'n_particles' new particles,
    //                      randomly choose a particle from  the current
    //                      set with probability given by the particle
    //                      probabilities computed in Step 3.
    //                      Initialize the new particles (x,y,theta)
    //                      from the selected particle.
    //                      Note that particles in the current set that
    //                      have high probability may end up being
    //                      copied multiple times.
    //
    //                      Once you have a new list of particles, replace
    //                      the current list with the new one. Be sure
    //                      to release the memory for the current list
    //                      before you lose that pointer!
    //

    /*******************************************************************
    // TO DO: Complete and test Step 4
    //        You should see most particles disappear except for
    //        those that agree with the robot's measurements.
    //        Hopefully the largest cluster will be on and around
    //        the robot's actual location/direction.
    *******************************************************************/
    int resample_size = round(0.9*n_particles);
    int random_size = n_particles-resample_size;
    // generate resample_size uniform [0, 1] numbers and sort them
    double mu[resample_size];
    for (int i = 0; i < resample_size; i++)
    {
      mu[i] = drand48();
    }
    qsort(mu, resample_size, sizeof(double), compare);
    // generate new list of n_particles from this
    p = list;
    int j = 0;
    double running_total_probability = 0;
    struct particle *newlist = NULL;
    // add resampled particles to list
    for (int i = 0; i < resample_size; i++)
    {
      double threshold = mu[i];
      while (running_total_probability <= threshold)
      {
        running_total_probability += p->prob;
        if (p->next != NULL)
        {
          p = p->next;
          j++;
        }
      }
      //duplicate the particle
      struct particle* resampled_p = initRobot(map, sx, sy);
      resampled_p->prob = 1/(double) n_particles;
      resampled_p->theta = p->theta;
      resampled_p->x = p->x;
      resampled_p->y = p->y;
      // add it to the new list
      resampled_p->next = newlist;
      newlist = resampled_p;
    }
    // add random particles to list
    int total_random_samples = 0;
    while (total_random_samples < random_size)
    {
      //printf("Still inside while loop, total_random_samples: %d, iteration_count: %d\n", total_random_samples, iteration_count);
      struct particle* random_p = initRobot(map, sx, sy);
      computeLikelihood(random_p, robot, 20);
      if (random_p->prob > 0.00002)
      {
        total_random_samples++;
        random_p->next = newlist;
        newlist = random_p;
      }
    }
    // for (int i = 0; i < random_size-total_random_samples; i++)
    // {
    //   struct particle* random_p = initRobot(map, sx, sy);
    //   random_p->prob = 1/(double) n_particles;
    //   // add it to the new list

    //   random_p->next = newlist;
    //   newlist = random_p;
    // }
    // deleteList
    deleteList(list);
    // set list to new list
    list = newlist;
  } // End if (!first_frame)

  /***************************************************
   OpenGL stuff
   You DO NOT need to read code below here. It only
   takes care of updating the screen.
  ***************************************************/
  if (RESETflag) // If user pressed r, reset particles
  {
    deleteList(list);
    list = NULL;
    initParticles();
    RESETflag = 0;
  }
  renderFrame(map, map_b, sx, sy, robot, list);

  // Clear the screen and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  glGenTextures(1, &texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sx, sy, 0, GL_RGB, GL_UNSIGNED_BYTE, map_b);

  // Draw box bounding the viewing area
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(0.0, 100.0, 0.0);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(800.0, 100.0, 0.0);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(800.0, 700.0, 0.0);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(0.0, 700.0, 0.0);
  glEnd();

  p = list;
  max = 0;
  while (p != NULL)
  {
    if (p->prob > max)
    {
      max = p->prob;
      pmax = p;
    }
    p = p->next;
  }

  if (!first_frame)
  {
    sprintf(&line[0], "X=%3.2f, Y=%3.2f, th=%3.2f, EstX=%3.2f, EstY=%3.2f, Est_th=%3.2f, Error=%f", robot->x, robot->y, robot->theta,
            pmax->x, pmax->y, pmax->theta, sqrt(((robot->x - pmax->x) * (robot->x - pmax->x)) + ((robot->y - pmax->y) * (robot->y - pmax->y))));
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(5, 22);
    for (int i = 0; i < strlen(&line[0]); i++)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, (int)line[i]);
  }

  // Make sure all OpenGL commands are executed
  glFlush();
  // Swap buffers to enable smooth animation
  glutSwapBuffers();

  glDeleteTextures(1, &texture);

  // Tell glut window to update ls itself
  glutSetWindow(windowID);
  glutPostRedisplay();

  if (first_frame)
  {
    fprintf(stderr, "All set! press enter to start\n");
    fgets(&line[0], 1, stdin);
    first_frame = 0;
  }
}

/*********************************************************************
 OpenGL and display stuff follows, you do not need to read code
 below this line.
*********************************************************************/
// Initialize glut and create a window with the specified caption
void initGlut(char *winName)
{
  // Set video mode: double-buffered, color, depth-buffered
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  // Create window
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(Win[0], Win[1]);
  windowID = glutCreateWindow(winName);

  // Setup callback functions to handle window-related events.
  // In particular, OpenGL has to be informed of which functions
  // to call when the image needs to be refreshed, and when the
  // image window is being resized.
  glutReshapeFunc(WindowReshape);      // Call WindowReshape whenever window resized
  glutDisplayFunc(ParticleFilterLoop); // Main display function is also the main loop
  glutKeyboardFunc(kbHandler);
}

void kbHandler(unsigned char key, int x, int y)
{
  if (key == 'r')
  {
    RESETflag = 1;
  }
  if (key == 'q')
  {
    deleteList(list);
    free(map);
    free(map_b);
    exit(0);
  }
}

void WindowReshape(int w, int h)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity(); // Initialize with identity matrix
  gluOrtho2D(0, 800, 800, 0);
  glViewport(0, 0, w, h);
  Win[0] = w;
  Win[1] = h;
}
