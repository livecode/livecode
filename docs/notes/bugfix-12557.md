# Images can draw incorrectly depending on coordinates.

Adjacent objects on a stack can sometimes be drawn with a gap between them when drawn at non-integer scales (For instance, on a Windows desktop with text scaling set to 150%). To address this issue we have tweaked the drawing code to ensure that control rects are aligned to integer device coordinates, which will eliminate this problem.