public class Ground implements IObject {

  public Material material1;
  public Material material2;

  public Ground(Material material1, Material material2) {
    this.material1 = material1;
    this.material2 = material2;
  }

  public boolean intersect(double[] o, double[] d, boolean primaryRay,
      double maxTime, double[][] temps, Intersection intersection) {

    if (d[1] == 0) {
      return false;
    }

    intersection.time = -o[1] / d[1];

    if (intersection.time >= Main.EPSILON && intersection.time <= maxTime) {

      if (!primaryRay) {
        return true;
      }

      Vec.assign(intersection.hit, o[0] + d[0] * intersection.time, 0,
          o[2] + d[2] * intersection.time);
      Vec.assign(intersection.normal, 0, 1, 0);

      long a = ((long)Math.floor(intersection.hit[0] 
          * Main.INVERSE_GROUND_SQUARE_SIZE)) & 1;
      long b = ((long)Math.floor(intersection.hit[2] 
          * Main.INVERSE_GROUND_SQUARE_SIZE)) & 1;
      intersection.material = (((a + b) & 1) == 0) ? material2 : material1;

      return true;
    }

    return false;
  }
}
