public final class Materials {

  private Materials() {
  }

  public static final Material RED_PLASTIC = createPlastic(1, 0, 0);
  public static final Material YELLOW_PLASTIC = createPlastic(1, 1, 0);

  public static final Material TORSO = createPlastic(
      createColor(0xE51715, 1.05));
  public static final Material SKIN = createPlastic(
      createColor(0xF2ADAB, 1.05));
  public static final Material EYE = createPlastic(
      createColor(0x1E1B94, 1.4));
  public static final Material HAIR = createPlastic(
      createColor(0x261117, 1.4));

  public static final Material BLUE_MATTE = createMatte(0, 0, 1);
  public static final Material GREEN_MATTE = createMatte(0, 1, 0);
  public static final Material YELLOW_MATTE = createMatte(1, 1, 0);
  public static final Material WHITE_MATTE = createMatte(1, 1, 1);
  
  public static final Material YELLOW_METAL = createMetal(1, 1, 0);
  public static final Material PURPLE_METAL = createMetal(1, 0, 1);
  public static final Material BLUE_METAL = createMetal(0, 0, 1);
  public static final Material CYAN_METAL = createMetal(0, 1, 1);
  public static final Material RED_METAL = createMetal(1, 0, 0);

  public static final Material MIRROR = createPolishedMetal(1, 1, 1);

  public static Material createMetal(double... color) {
    return new Material(
      0.1,
      1.0,
      0.7,
      1.0,
      1.0,
      20.0,
      color,
      color,
      color
    );
  }

  public static Material createPolishedMetal(double... color) {
    return new Material(
      0.0,
      0.0,
      0.0,
      1.0,
      1.0,
      20.0,
      color,
      color,
      color
    );
  }

  public static double[] createColor(int hexColor, double scale) {
    double[] color = new double[3];
    createColor(color, hexColor, scale);
    return color;
  }

  public static void createColor(
      double[] color, int hexColor, double scale) {
    for(int i = 2; i >= 0; i--) {
      int intensity = hexColor & 0xFF;
      hexColor >>= 8;
      color[i] = Math.pow(scale * intensity / 255.0, Main.GAMMA);
    }
  }

  public static Material createPlastic(double... color) {
    return new Material(
      0.016988052089250049403595337516742,
      1.0,
      1.0,
      1.0,
      0.0,
      10.0,
      color,
      new double[] { 1, 1, 1 },
      new double[] { 0, 0, 0 }
    );
  }

  public static Material createMatte(double... color) {
    return new Material(
      0.13320851318429970246653555493722,
      1.0,
      1.5,
      0.0,
      0.0,
      0.0,
      color,
      new double[] { 0, 0, 0 },
      new double[] { 0, 0, 0 }
    );
  }
}
