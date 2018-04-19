package android.print;

import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.print.PrintDocumentAdapter;

import java.io.File;

public class PdfPrint {

    private static final String TAG = PdfPrint.class.getSimpleName();
    private PrintAttributes m_attributes;
    private PrintDocumentAdapter m_print_adapter;
    private File m_path;

    public PdfPrint(int pWidth, int pHeight) {
       PrintAttributes t_attributes = new PrintAttributes.Builder()
                 .setMediaSize(new PrintAttributes.MediaSize("LiveCodeCustomSize","Custom Size", pWidth/72*1000, pHeight/72*1000))
                 .setResolution(new PrintAttributes.Resolution("pdf", "pdf", 300, 300))
                 .setMinMargins(new PrintAttributes.Margins(1000,1000,1000,1000)).build();
        m_attributes = t_attributes;
        Log.w(TAG,"PdfPrint");
    }

    public class PdfPrintLayoutResultCallback extends android.print.PrintDocumentAdapter.LayoutResultCallback {
      @Override
      public void onLayoutFinished(PrintDocumentInfo info, boolean changed) {
          m_print_adapter.onWrite(new PageRange[]{PageRange.ALL_PAGES}, getOutputFile(), new CancellationSignal(), new PrintDocumentAdapter.WriteResultCallback() {
              @Override
              public void onWriteFinished(PageRange[] pages) {
                  super.onWriteFinished(pages);
              }
          });
      }
      
      @Override
      public void onLayoutFailed(CharSequence error) {}
      
      @Override
      public void onLayoutCancelled() {}
    }
    
    public void print(final PrintDocumentAdapter p_print_adapter, File p_path) {
        m_print_adapter = p_print_adapter;
        m_path = p_path;
        m_print_adapter.onLayout(null, m_attributes, null, new PdfPrintLayoutResultCallback(), null);
    }

    private ParcelFileDescriptor getOutputFile() {
        try {
            return ParcelFileDescriptor.open(m_path, ParcelFileDescriptor.MODE_READ_WRITE);
        } catch (Exception e) {
            Log.e(TAG, "Failed to open ParcelFileDescriptor", e);
        }
        return null;
    }
}
