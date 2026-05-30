public class SkyMaterial extends Material {

  private double[] minColor = Materials.createColor(0xBDBDFF, 1.0);
  private double[] maxColor = Materials.createColor(0x2223F6, 1.0);

  public SkyMaterial() {
    super(1, 0, 0, 0, 0, 0, new double[3], new double[3], new double[3]);
  }

  @Override
  public void update(double[] hitPoint) {
    Vec.interpolate(diffuseColor, minColor, maxColor, hitPoint[1] * 1E-6);
  }  
}
