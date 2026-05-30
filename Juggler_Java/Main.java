import java.awt.image.*;
import javax.imageio.*;
import java.io.*;

public class Main {

  public static final int SQRT_SAMPLES = 32;

  public static final int IMAGE_SCALE = 1;
  public static final int WIDTH = 1920 / IMAGE_SCALE;
  public static final int HEIGHT = 1080 / IMAGE_SCALE;
  public static final double VIRTUAL_SCREEN_WIDTH = 100;
  public static final int FRAMES = 30;

  public static final double[] EYE = { 2, 100, -2 };
  public static final double[] LOOK = { 1000, 77, -1000 };
  public static final double[] LIGHT = { -564, 686, 147 };
  public static final double[] LIGHT_COLOR = { 1, 1, 1 };
  public static final double LIGHT_RADIUS = 10.0;
  public static final double[] AMBIENT_COLOR = { 1, 1, 1 };
  public static double DISTANCE_TO_VIRTUAL_SCREEN = 50.0;
  public static double GROUND_SQUARE_SIZE = 107.0;

  public static final String IMAGE_TYPE = "png";
  public static final boolean RENDER_IN_WINDOW = true;

  public static final double GAMMA = 2.2;

  public static final int SAMPLES = SQRT_SAMPLES * SQRT_SAMPLES;
  public static final double INVERSE_SQRT_SAMPLES = 1.0 / SQRT_SAMPLES;
  public static final double INVERSE_SAMPLES = 1.0 / SAMPLES;
  public static final double INVERSE_GAMMA = 1.0 / GAMMA;
  public static final double HALF_WIDTH = WIDTH / 2.0;
  public static final double HALF_HEIGHT = HEIGHT / 2.0;
  public static final double VIRTUAL_SCREEN_RATIO
      = VIRTUAL_SCREEN_WIDTH / WIDTH;
  public static double INVERSE_GROUND_SQUARE_SIZE = 1.0 / GROUND_SQUARE_SIZE;

  public static final boolean AMBIENT_OCCLUSION = true;
  public static final double MAX_OCCLUSION_DISTANCE = 100.0;
  public static final double RADIANCE_SCALE = 1.0;
  public static final double EPSILON = 1E-6;
  public static final int MAX_DEPTH = 10;
  public static final double MIN_COLOR_INTENSITY = 1.0 / 256.0;

  public static final long SECOND_MILLIS = 1000L;
  public static final long MINUTE_MILLIS = 60 * SECOND_MILLIS;
  public static final long HOUR_MILLIS = 60 * MINUTE_MILLIS;

  public static final double JUGGLE_X0 = -182;
  public static final double JUGGLE_X1 = -108;
  public static final double JUGGLE_Y0 = 88;
  public static final double JUGGLE_H_Y = 184;

  public static final double JUGGLE_H_VX = (JUGGLE_X0 - JUGGLE_X1) / 60.0;
  public static final double JUGGLE_L_VX = (JUGGLE_X1 - JUGGLE_X0) / 30.0;

  public static final double JUGGLE_H_H = JUGGLE_H_Y - JUGGLE_Y0;
  public static final double JUGGLE_H_VY = 4.0 * JUGGLE_H_H / 60.0;
  public static final double JUGGLE_G = JUGGLE_H_VY * JUGGLE_H_VY
      / (2.0 * JUGGLE_H_H);

  public static final double JUGGLE_L_VY = 0.5 * JUGGLE_G * 30.0;

  public static final double HIPS_MAX_Y = 85;
  public static final double HIPS_MIN_Y = 81;

  public static final double HIPS_ANGLE_MULTIPLIER = 2.0 * Math.PI / 30.0;

  private RenderFrame renderFrame;
  private BufferedImage image;
  private int runningCount;
  private int rowIndex;

  public void launch() throws Throwable {
    
    image = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_INT_RGB);
    if (RENDER_IN_WINDOW) {
      renderFrame = new RenderFrame(image);
    }
    
    int processors = Runtime.getRuntime().availableProcessors();    

    for(int frameIndex = 0; frameIndex < FRAMES; frameIndex++) {

      if (RENDER_IN_WINDOW) {
        renderFrame.setTitle("Amiga Juggler [" + frameIndex + "]");
      }

      resetRowIndex();
      updateRunningCount(processors);
      for(int i = 0; i < processors; i++) {
        final int _frameIndex = frameIndex;
        new Thread(Integer.toString(i)) {
          @Override
          public void run() {
            render(_frameIndex);
            updateRunningCount(-1);
          }
        }.start();
      }

      synchronized(this) {
        while(runningCount != 0) {
          wait();
        }
      }

      saveImage("juggler" + ((frameIndex < 10) ? "0" : "") + frameIndex);
    }
  }
 
  private void render(int frameIndex) {

    IObject[] scene = createScene();

    double[] u = new double[3];
    double[] v = new double[3];
    double[] w = new double[3];
    double[] c = new double[3];
    double[] p = new double[3];
    double[] d = new double[3];
    double[] l = new double[3];
    double[] l2 = new double[3];
    double[] light = new double[3];
    double[] r = new double[3];
    double[] ar = new double[3];
    double[] f = new double[3];
    double[] o = new double[3];
    double[][] temps = new double[16][3];
    Intersection intersection = new Intersection();
    Intersection bestIntersection = new Intersection();

    Vec.constructUnitVector(w, EYE, LOOK);
    Vec.ray(c, EYE, w, -DISTANCE_TO_VIRTUAL_SCREEN);
    Vec.onb(u, v, w);

    RandomRays randomRays = new RandomRays();
    RandomDoubles randomDoubles = new RandomDoubles();
    RandomTimes randomTimes = new RandomTimes();
    int[] pixels = new int[WIDTH];
    double[] pixel = new double[3];

    while(true) {

      int y = getNextRowIndex();
      if (y >= HEIGHT) {
        return;
      }

      for(int x = 0; x < WIDTH; x++) {

        Vec.assign(pixel, 0, 0, 0);

        double[] times = randomTimes.random();
        int timesIndex = 0;

        for(int i = 0; i < SQRT_SAMPLES; i++) {

          double b = y + INVERSE_SQRT_SAMPLES * i;
          if (SQRT_SAMPLES == 1) {
            b += 0.5;
          } else {
            b += randomDoubles.random();
          }

          b = VIRTUAL_SCREEN_RATIO * (HALF_HEIGHT - b);

          for(int j = 0; j < SQRT_SAMPLES; j++) {
            double a = x + INVERSE_SQRT_SAMPLES * j;
            if (SQRT_SAMPLES == 1) {
              a += 0.5;
            } else {
              a += randomDoubles.random();
            }

            a = VIRTUAL_SCREEN_RATIO * (a - HALF_WIDTH);

            Vec.map(p, c, u, v, a, b);
            Vec.constructUnitVector(d, p, EYE);

            Vec.assign(o, EYE);
            Vec.assign(f, RADIANCE_SCALE, RADIANCE_SCALE, RADIANCE_SCALE);

            updateScene(frameIndex, times[timesIndex++], scene, temps);

            reflectionLoop: for(int m = 0; m < MAX_DEPTH; m++) {

              boolean hit = false;
              bestIntersection.time = Double.POSITIVE_INFINITY;
              for(int k = scene.length - 1; k >= 0; k--) {
                IObject object = scene[k];
                if (object.intersect(o, d, true, Double.POSITIVE_INFINITY,
                    temps, intersection)) {
                  if (intersection.time < bestIntersection.time) {
                    hit = true;
                    bestIntersection.time = intersection.time;
                    Vec.assign(bestIntersection.normal, intersection.normal);
                    Vec.assign(bestIntersection.hit, intersection.hit);
                    bestIntersection.material = intersection.material;
                  }
                }
              }

              if (hit) {

                bestIntersection.material.update(bestIntersection.hit);

                if (Vec.dot(bestIntersection.normal, d) >= 0) {
                  Vec.negate(bestIntersection.normal);
                }

                if (bestIntersection.material.ambientWeight > 0) {
                  double ambientPercent = 1.0;

                  if (AMBIENT_OCCLUSION
                      && bestIntersection.material.ambientOcclusionPercent > 0) {

                    randomRays.randomRay(ar);
                    if (Vec.dot(ar, bestIntersection.normal) < 0) {
                      Vec.negate(ar);
                    }

                    for(int k = scene.length - 1; k >= 0; k--) {
                      IObject object = scene[k];
                      if (object.intersect(bestIntersection.hit, ar, false,
                          MAX_OCCLUSION_DISTANCE, temps, intersection)) {
                        ambientPercent = 1.0
                            - bestIntersection.material.ambientOcclusionPercent;
                        break;
                      }
                    }
                  }

                  for(int k = 0; k < 3; k++) {
                    pixel[k] += bestIntersection.material.ambientWeight
                        * bestIntersection.material.diffuseColor[k]
                        * f[k]
                        * AMBIENT_COLOR[k]
                        * ambientPercent;
                  }
                }

                Vec.assign(light, LIGHT);
                if (LIGHT_RADIUS > 0) {
                  Vec.subtract(l2, bestIntersection.hit, light);
                  randomRays.randomRay(l);
                  if (Vec.dot(l, l2) < 0) {
                    Vec.negate(l);
                  }
                  Vec.ray(light, LIGHT, l, LIGHT_RADIUS);
                } 
                Vec.constructUnitVector(l, light, bestIntersection.hit);

                double nDotl = Vec.dot(l, bestIntersection.normal);
                if (nDotl > 0) {

                  double maxTime = Vec.distance(bestIntersection.hit, light);

                  boolean illuminated = true;
                  for(int k = scene.length - 1; k >= 0; k--) {
                    IObject object = scene[k];
                    if (object.intersect(bestIntersection.hit, l, false,
                        maxTime, temps, intersection)) {
                      illuminated = false;
                      break;
                    }
                  }

                  if (illuminated) {

                    if (bestIntersection.material.diffuseWeight > 0) {
                      for(int k = 0; k < 3; k++) {
                        pixel[k] += bestIntersection.material.diffuseWeight
                            * bestIntersection.material.diffuseColor[k]
                            * f[k]
                            * LIGHT_COLOR[k]
                            * nDotl;
                      }
                    }

                    if (bestIntersection.material.specularWeight > 0) {

                      Vec.scale(r, bestIntersection.normal, 2.0 * nDotl);
                      Vec.subtract(r, l);
                      double rDotMd = Vec.dotNegative(r, d);
                      if (rDotMd > 0) {
                        for(int k = 0; k < 3; k++) {
                          pixel[k] += bestIntersection.material.specularWeight
                              * Math.pow(rDotMd,
                                  bestIntersection.material.shininess)
                              * f[k]
                              * LIGHT_COLOR[k]
                              * nDotl
                              * bestIntersection.material.highlightColor[k];
                        }
                      }
                    }
                  }
                }

                if (bestIntersection.material.reflectionWeight > 0) {
                  boolean aboveMinColorIntensity = false;
                  for(int k = 0; k < 3; k++) {
                    f[k] *= bestIntersection.material.reflectionWeight
                        * bestIntersection.material.reflectionColor[k];
                    if (f[k] >= MIN_COLOR_INTENSITY) {                      
                      aboveMinColorIntensity = true;
                    }
                  }
                  if (!aboveMinColorIntensity) {
                    break reflectionLoop;
                  }

                  Vec.ray(d, bestIntersection.normal,
                      -2 * Vec.dot(bestIntersection.normal, d));
                  Vec.assign(o, bestIntersection.hit);
                } else {
                  break reflectionLoop;
                }

              } else {
                break reflectionLoop;
              }
            }
          }
        }

        int value = 0;
        for(int i = 0; i < 3; i++) {
          int intensity = (int)Math.round(255
              * Math.pow(pixel[i] * INVERSE_SAMPLES, INVERSE_GAMMA));
          if (intensity < 0) {
            intensity = 0;
          } else if (intensity > 255) {
            intensity = 255;
          }
          value <<= 8;
          value |= intensity;
        }

        pixels[x] = value;
      }

      rowCompleted(y, pixels);
    }
  }

  private IObject[] createScene() {

    // juggling balls 2 -- 4
    // body (hips to chest) 5 -- 12
    // head 13
    // neck 14
    // left leg (15 -- 31)
    // right leg (32 -- 48)
    // left arm (49 -- 65)
    // right arm (66 -- 82)
    // left eye 83
    // right eye 34
    // hair 85

    IObject[] scene = new IObject[86];
    scene[0] = new Ground(Materials.YELLOW_MATTE, Materials.GREEN_MATTE);
    scene[1] = new Sphere(0, 0, 0, 1E6, new SkyMaterial());
    for(int i = 2; i <= 4; i++) {
      scene[i] = new Sphere(110, 0, 0, 14, Materials.MIRROR);
    }
    for(int i = 5; i <= 12; i++) {
      double percent = (i - 5) / 7.0;
      scene[i] = new Sphere(151, 85 + 32 * percent, -151, 16 + 4 * percent,
          Materials.TORSO);
    }
    scene[13] = new Sphere(151, 155, -151, 14, Materials.SKIN);
    scene[14] = new Sphere(151, 140, -151, 5, Materials.SKIN);

    for(int i = 15; i <= 22; i++) {
      for(int j = 0; j < 4; j++) {
        scene[i + 17 * j] = new Sphere(0, 0, 0, 2.5 + 2.5 * (i - 15) / 7.0,
            Materials.SKIN);
      }
    }
    for(int i = 23; i <= 31; i++) {
      for(int j = 0; j < 4; j++) {
        scene[i + 17 * j] = new Sphere(0, 0, 0, 5, Materials.SKIN);
      }
    }
    scene[83] = new Sphere(142, 154, -144, 4, Materials.EYE);
    scene[84] = new Sphere(142, 154, -144, 4, Materials.EYE);
    scene[85] = new Sphere(152, 156, -151, 14, Materials.HAIR);

    return scene;
  }

  private void updateScene(
      int frameIndex, double t, IObject[] scene, double[][] temps) {

    // mirrored juggling balls (2 -- 4)
    double T = (30 + frameIndex + t);
    Sphere sphere = (Sphere)scene[3];
    sphere.center[2] = JUGGLE_X1 + JUGGLE_H_VX * T;
    sphere.center[1] = JUGGLE_Y0 + (JUGGLE_H_VY - 0.5 * JUGGLE_G * T) * T;

    T = frameIndex + t;
    sphere = (Sphere)scene[4];
    sphere.center[2] = JUGGLE_X1 + JUGGLE_H_VX * T;
    sphere.center[1] = JUGGLE_Y0 + (JUGGLE_H_VY - 0.5 * JUGGLE_G * T) * T;

    sphere = (Sphere)scene[2];
    sphere.center[2] = JUGGLE_X0 + JUGGLE_L_VX * T;
    sphere.center[1] = JUGGLE_Y0 + (JUGGLE_L_VY - 0.5 * JUGGLE_G * T) * T;

    // body (hips to chest) 5 -- 12
    double angle = HIPS_ANGLE_MULTIPLIER * T;
    double oscillation = 0.5 * (1.0 + Math.cos(angle));

    double[] o = temps[5];
    o[0] = 151;
    o[1] = HIPS_MIN_Y + (HIPS_MAX_Y - HIPS_MIN_Y) * oscillation;
    o[2] = -151;
    
    double[] u = temps[6];
    double[] v = temps[7];
    double[] w = temps[8];

    v[0] = 0;
    v[1] = 70;
    v[2] = (HIPS_MIN_Y - HIPS_MAX_Y) * Math.sin(angle);
    Vec.normalize(v);

    u[0] = 0;
    u[1] = v[2];
    u[2] = -v[1];

    w[0] = 1;
    w[1] = 0;
    w[2] = 0;

    for(int i = 5; i <= 12; i++) {
      double percent = (i - 5) / 7.0;
      sphere = (Sphere)scene[i];
      Vec.ray(sphere.center, o, v, 32 * percent);
    }
    sphere = (Sphere)scene[13];
    Vec.ray(sphere.center, o, v, 70);
    sphere = (Sphere)scene[14];
    Vec.ray(sphere.center, o, v, 55);

    // left leg (15 -- 32)
    double[] p = temps[9];
    p[0] = 159;
    p[1] = 2.5;
    p[2] = -133;
    double[] q = temps[10];
    Vec.mapYZ(q, o, v, u, -9, -16);
    updateAppendage(scene, 15, p, q, u, 42.58, 34.07, 8, 8, temps);

    // right leg (32 -- 48)
    p[0] = 139;
    p[1] = 2.5;
    p[2] = -164;
    Vec.mapYZ(q, o, v, u, -9, 16);
    updateAppendage(scene, 32, p, q, u, 42.58, 34.07, 8, 8, temps);

    // left arm (49 -- 65)
    double[] n = temps[11];
    double armAngle = -0.35 * oscillation;
    p[0] = 69 + 41 * Math.cos(armAngle);
    p[1] = 60 - 41 * Math.sin(armAngle);
    p[2] = -108;
    Vec.mapYZ(q, o, v, u, 45, -19);    
    Vec.mapYZ(n, o, v, u, 45.41217, -19.91111);
    Vec.subtract(n, q);
    updateAppendage(scene, 49, p, q, n, 44.294, 46.098, 8, 8, temps);

    // right arm (66 -- 82)
    p[2] = -182;
    Vec.mapYZ(q, o, v, u, 45, 19);
    Vec.mapYZ(n, o, v, u, 45.41217, 19.91111);
    Vec.subtract(n, q, n);
    updateAppendage(scene, 66, p, q, n, 44.294, 46.098, 8, 8, temps);

    // left eye (83)
    sphere = (Sphere)scene[83];
    Vec.mapYZ(sphere.center, o, v, u, 69, -7);
    sphere.center[0] = 142;

    // right eye (84)
    sphere = (Sphere)scene[84];
    Vec.mapYZ(sphere.center, o, v, u, 69, 7);
    sphere.center[0] = 142; 

    // hair (85)
    sphere = (Sphere)scene[85];
    Vec.ray(sphere.center, o, v, 71);
    sphere.center[0] = 152;
  }

  private void updateAppendage(
      IObject[] scene, int sceneIndex,
      double[] p, double[] q, double[] w, double A, double B,
      int countA, int countB, double[][] temps) {

    double[] U = temps[0];
    double[] V = temps[1];
    double[] W = temps[2];
    double[] j = temps[3];
    double[] d = temps[4];

    Vec.subtract(V, q, p);
    double D = Vec.magnitude(V);
    double inverseD = 1.0 / D;
    Vec.scale(V, inverseD);

    Vec.normalize(W, w);
    Vec.cross(U, V, W);

    double A2 = A * A;

    double y = 0.5 * inverseD * (A2 - B * B + D * D);
    double square = A2 - y * y;
    if (square < 0) {
      throw new RuntimeException("Unable to construct appendage.");
    }
    double x = Math.sqrt(square);

    Vec.map(j, p, U, V, x, y);

    Vec.subtract(d, j, p);
    Vec.scale(d, 1.0 / countA);
    for(int i = 0; i <= countA; i++) {
      double[] center = ((Sphere)scene[sceneIndex + i]).center;
      Vec.ray(center, p, d, i);
    }

    Vec.subtract(d, j, q);
    Vec.scale(d, 1.0 / countB);
    for(int i = 0; i < countB; i++) {
      double[] center = ((Sphere)scene[countA + 1 + sceneIndex + i]).center;
      Vec.ray(center, q, d, i);      
    }
  }

  private synchronized void updateRunningCount(int dx) {
    runningCount += dx;
    if (runningCount == 0) {
      notifyAll();
    }
  }

  private synchronized void resetRowIndex() {
    rowIndex = 0;
  }

  private synchronized int getNextRowIndex() {
    return rowIndex++;
  }

  private synchronized void rowCompleted(int rowIndex, int[] pixels) {
    image.setRGB(0, rowIndex, WIDTH, 1, pixels, 0, WIDTH);
    if (RENDER_IN_WINDOW) {
      renderFrame.imageUpdated();
    }
  }

  private void saveImage(String name) throws Throwable {
    ImageIO.write(image, IMAGE_TYPE, new File(name + "." + IMAGE_TYPE));
  }

  public static void main(String... args) throws Throwable {

    long startTime = System.currentTimeMillis();

    Main main = new Main();
    main.launch();

    long interval = System.currentTimeMillis() - startTime;
    long hours = interval / HOUR_MILLIS;
    interval %= HOUR_MILLIS;
    long minutes = interval / MINUTE_MILLIS;
    interval %= MINUTE_MILLIS;
    long seconds = interval / SECOND_MILLIS;
    interval %= SECOND_MILLIS;
    System.out.format("%d hour%s, %d minute%s, %d second%s, %d millisecond%s%n",
        hours, hours == 1 ? "" : "s",
        minutes, minutes == 1 ? "" : "s",
        seconds, seconds == 1 ? "" : "s",
        interval, interval == 1 ? "" : "s");
  }

}
