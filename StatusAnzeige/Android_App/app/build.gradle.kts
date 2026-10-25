plugins {
    id("com.android.application")
}

android {
    namespace = "de.technox90.statusanzeige"
    compileSdk = 35

    defaultConfig {
        applicationId = "de.technox90.statusanzeige"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }
}
