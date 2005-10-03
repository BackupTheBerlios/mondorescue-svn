--- ../../branches/devel/mondo/common/newt-specific.c	2004-09-09 09:38:52.000000000 -0700
+++ common/newt-specific.c	2004-10-02 16:51:09.881438200 -0700
@@ -423,14 +423,26 @@
 
   printf ("---FATALERROR--- %s\n", error_string);
   system("cat /var/log/mondo-archive.log | gzip -9 > /tmp/MA.log.gz 2> /dev/null");
-  printf("If you require technical support, please contact the mailing list.\n");
-  printf("See http://www.mondorescue.org for details.\n");
+  if (!strstr(VERSION, "cvs") && !strstr(VERSION, "svn"))
+    {
+      printf("Please try the snapshot (the version with 'cvs' and the date in its filename)");
+      printf("to see if that fixes the problem. Please don't bother the mailing list with");
+      printf("your problem UNTIL you've tried the snapshot. The snapshot contains bugfixes");
+      printf("which might help you. Go to http://www.mondorescue.org/download/download.html");
+      printf("For more information.\n");
+      log_msg(0, "Please DON'T contact the mailing list. Try the SNAPSHOTS.");
+    }
+  else
+    {
+      printf("If you require technical support, please contact the mailing list.\n");
+      printf("See http://www.mondorescue.org for details.\n");
+      printf("The list's members can help you, if you attach that file to your e-mail.\n");
+    }
   printf("Log file: %s\n", MONDO_LOGFILE);
   //  printf("VERSION=%s\n", VERSION);
   if (does_file_exist("/tmp/MA.log.gz"))
     {
       printf("FYI, I have gzipped the log and saved it to /tmp/MA.log.gz\n");
-      printf("The list's members can help you, if you attach that file to your e-mail.\n");
     }
   printf ("Mondo has aborted.\n");
   register_pid(0, "mondo"); // finish() does this too, FYI
