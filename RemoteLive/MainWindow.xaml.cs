using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Drawing.Imaging;
using System.IO;
using Encoder = System.Drawing.Imaging.Encoder;
using System.Diagnostics;
using System.Net.Sockets;
using System.Threading;
using Point = System.Drawing.Point;

namespace RemoteLive
{
    
    public partial class MainWindow : Window
    {
        private static ImageCodecInfo GetEncoder(string format)
        {
            int j;
            ImageCodecInfo[] encoders;
            encoders = ImageCodecInfo.GetImageEncoders();
            for (j = 0; j < encoders.Length; j++)
            {
                if (encoders[j].MimeType == format)
                    return encoders[j];
            }

            return null;
        }
        private byte[] CompressionImage(Bitmap bm, long quality)
        {

            ImageCodecInfo CodecInfo = GetEncoder("image/jpeg");
            System.Drawing.Imaging.Encoder myEncoder = System.Drawing.Imaging.Encoder.Quality;
            EncoderParameters myEncoderParameters = new EncoderParameters(1);
            EncoderParameter myEncoderParameter = new EncoderParameter(myEncoder, quality);
            myEncoderParameters.Param[0] = myEncoderParameter;
            using (MemoryStream ms = new MemoryStream())
            {
                bm.Save(ms, CodecInfo, myEncoderParameters);
                myEncoderParameters.Dispose();
                myEncoderParameter.Dispose();
                return ms.ToArray();
            }


        }
        public int Length = 0;
        public int Pitch = 0;
        public IntPtr lpBuffer = (IntPtr)0;
        private byte[] recvBuf = new byte[10];

        [DllImport("user32.dll", EntryPoint = "GetCursorPos")]
        public static extern bool GetCursorPos(ref Point lpPoint);
        private WriteableBitmap Sr = null;
        private Bitmap Handle = null;
        [DllImport("Kernel32.dll")]
        extern static public void RtlMoveMemory(IntPtr Destination, IntPtr Source, int Length);
        [DllImport("Patch.dll")]
        extern static public bool Init();
        [DllImport("Patch.dll")]
        extern static public bool CaptureImage(IntPtr pData, ref int nLen, ref int Pitch);
        bool connected = false;
        TcpClient tcl = new TcpClient();
        NetworkStream sl = null;
        public MainWindow()
        {
            InitializeComponent();
            Init();
            lpBuffer = Marshal.AllocHGlobal(4096 * 2304 * 4);

            Loaded += (e, v) =>
            {

                //       CaptureImage(lpBuffer, ref Length, ref Pitch);


                //       Sr = new WriteableBitmap(Pitch / 4, Length / Pitch, 96, 96, PixelFormats.Bgra32, null);


                //      Output.Source = Sr;

                //      Sr.Lock();

                //  Marshal.Copy();
                //      RtlMoveMemory(Sr.BackBuffer, lpBuffer, Length);
                //       Sr.AddDirtyRect(new Int32Rect(0, 0, (int)Sr.Width, (int)Sr.Height));
                //      Sr.Unlock();
                Window1 w1 = new Window1();
                w1.ShowDialog();

                string ip = w1.IP.Text;
                Thread nth = new Thread(() =>
                {
                    while (true)
                    {
                        try
                        {
                            if(ip == "IP")
                            tcl.Connect("192.168.134.64", 10436);
                            else tcl.Connect(ip, 10436);
                        }
                        catch (Exception)
                        {
                            continue;
                        }
                        sl = tcl.GetStream();
                        connected = true;

                        while (connected)
                        {
                            Thread.Sleep(1);
                        }

                        tcl.Dispose();
                        tcl = new TcpClient();
                    }
                })
                { IsBackground = true };
                nth.Start();

                Thread nth2 = new Thread(() =>
                {
                    while (true)
                    {
                        CaptureImage(lpBuffer, ref Length, ref Pitch);
                        if (Length == 0 || !connected)
                        {
                            continue;
                        }
                        int buf_size = (int)(tcl.SendBufferSize / 3d);
                        if (Length != 0)
                        {
                            System.Drawing.Point p2 = new System.Drawing.Point();
                            GetCursorPos(ref p2);
                            Bitmap bm = new Bitmap(Pitch / 4, Length / Pitch, Pitch, System.Drawing.Imaging.PixelFormat.Format32bppArgb, lpBuffer);

                            Graphics gw = Graphics.FromImage(bm);
                            gw.DrawEllipse(new System.Drawing.Pen(new System.Drawing.SolidBrush(System.Drawing.Color.Black)), new System.Drawing.Rectangle(p2.X - 6, p2.Y - 6, 12, 12));
                            gw.Dispose();

                            Bitmap n_buffer = new Bitmap(1600, 900);
                            Graphics g = Graphics.FromImage(n_buffer);
                            g.DrawImage(bm, 0, 0, 1600, 900);
                            var vceData = CompressionImage(n_buffer, 32);

                            g.Dispose();
                            n_buffer.Dispose();

                            for (int i = 0; i < vceData.Length; i += buf_size)
                            {
                                var lbuf = vceData.Skip(i).Take(buf_size).ToArray();
                                try
                                {
                                    sl.Write(lbuf, 0, lbuf.Length);
                                    int sc = sl.Read(recvBuf, 0, recvBuf.Length);
                                    if (sc == 0) { connected = false; continue; }
                                }
                                catch (Exception)
                                {
                                    connected = false; continue;
                                }
                            }
                            try
                            {
                                sl.Write(new byte[4] { 104, 105, 56, 33 }, 0, 4);
                                int size = sl.Read(recvBuf, 0, recvBuf.Length);
                            }
                            catch (Exception)
                            {
                                connected = false; continue;
                            }
                        }
                    }
                })
                { IsBackground = true };
                nth2.Start();


                DispatcherTimer t = new DispatcherTimer();
                t.Tick += (e, v) =>
                {
                    CaptureImage(lpBuffer, ref Length, ref Pitch);
                    if (Length == 0 || !connected)
                    {
                        return;
                    }


                        Sr.Lock();

                    int buf_size = (int)(tcl.SendBufferSize / 3d);
                    if (Length != 0)
                    {
                        Bitmap bm = new Bitmap(Pitch / 4, Length / Pitch, Pitch, System.Drawing.Imaging.PixelFormat.Format32bppArgb, lpBuffer);
                        var vceData = CompressionImage(bm, 18);

                        for (int i = 0; i < vceData.Length; i+= buf_size)
                        {
                            var lbuf = vceData.Skip(i).Take(buf_size).ToArray();
                            try
                            {
                                sl.Write(lbuf, 0, lbuf.Length);
                                int sc = sl.Read(recvBuf, 0, recvBuf.Length);
                                if (sc == 0) { connected = false; return; }
                            }
                            catch (Exception)
                            {
                                connected = false; return;
                            }
                        }
                        try
                        {
                            sl.Write(new byte[4] { 104, 105, 56, 33 }, 0, 4);
                            int size = sl.Read(recvBuf, 0, recvBuf.Length);
                        }
                        catch (Exception)
                        {
                            connected = false; return;
                        }
                    }

                     RtlMoveMemory(Sr.BackBuffer, lpBuffer, Length);

                  
                    Sr.AddDirtyRect(new Int32Rect(0, 0, (int)Sr.Width, (int)Sr.Height));
                    Sr.Unlock();
                   
                };

                t.Interval = TimeSpan.FromMilliseconds(100);
         //     t.Start();
            };
        }
    }
}
