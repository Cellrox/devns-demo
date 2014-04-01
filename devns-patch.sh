#!/bin/bash

for d in .repo build system/core; do
	if [ ! -d $d ]; then
		echo "This script must run from the AOSP top directory."
		exit 1
	fi
done

pushd build > /dev/null
git apply << "END"
diff --git a/target/product/core.mk b/target/product/core.mk
index 1d62eb8..aaf6a35 100644
--- a/target/product/core.mk
+++ b/target/product/core.mk
@@ -165,5 +165,12 @@ ifeq ($(HAVE_SELINUX),true)
         mac_permissions.xml
 endif
 
+# add devns demo
+PRODUCT_PACKAGES += \
+        devns \
+        devns_init \
+        devns_switch \
+		DevnsDemo
+
 $(call inherit-product, $(SRC_TARGET_DIR)/product/base.mk)
 
END
popd > /dev/null

pushd system/core > /dev/null
git apply << "END"
diff --git a/rootdir/init.rc b/rootdir/init.rc
index 0face12..1e1895a 100644
--- a/rootdir/init.rc
+++ b/rootdir/init.rc
@@ -515,3 +515,8 @@ service mdnsd /system/bin/mdnsd
     socket mdnsd stream 0660 mdnsr inet
     disabled
     oneshot
+
+service devns /system/bin/devns
+    class main
+    user root
+    oneshot
END
popd > /dev/null

