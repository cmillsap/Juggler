import java.util.*;

public class RandomTimes {

  private static volatile double[][] times;

  private int index;

  public RandomTimes() {
    synchronized(RandomTimes.class) {

      ArrayList<Double> doubles = new ArrayList<Double>();

      if (times == null) {
        times = new double[503][Main.SAMPLES];
        for(int i = 0; i < times.length; i++) {
          doubles.clear();
          for(int j = 0; j < Main.SAMPLES; j++) {
            doubles.add(Main.INVERSE_SAMPLES * (j + Math.random()));
          }
          for(int j = 0; j < Main.SAMPLES; j++) {
            times[i][j] = doubles.remove((int)(Math.random() * doubles.size()));
          }
        }
      }
      index = (int)(Math.random() * times.length);
    }
  }

  public double[] random() {
    if (index >= times.length) {
      index = 0;
    }
    return times[index++];
  }
}
