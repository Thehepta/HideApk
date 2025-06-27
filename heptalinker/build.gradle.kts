plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.kotlin.android)
    id("maven-publish")
}


android {
    namespace = "com.hepta.linker"
    compileSdk = 33

    defaultConfig {
        minSdk = 26
        version = "0.0.3"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags("")
                abiFilters ("armeabi-v7a", "arm64-v8a")

            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }

    publishing {
        singleVariant("release")
    }
    ndkVersion = "27.0.12077973"

}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}

afterEvaluate {
    publishing {
        publications {
            create<MavenPublication>("release") {
                groupId = "com.github.Thehepta"
                artifactId = "HeptaLinker"
                version = "0.0.4"

                // 指定 AAR 文件路径
                artifact(tasks.named("bundleReleaseAar"))

                // 可选：添加 pom 文件信息
                pom {
                    name.set("HeptaLinker")
                    description.set("An Android library for linking native code.")
                    url.set("https://github.com/Thehepta/HeptaLinker ")
                    licenses {
                        license {
                            name.set("MIT License")
                            url.set("https://opensource.org/licenses/MIT ")
                        }
                    }
                    developers {
                        developer {
                            name = "thehepta"
                            url = "https://github.com/thehepta"
                        }
                    }
                    scm {
                        url.set("https://github.com/thehepta/heptalinker ")
                    }
                }
            }
        }
    }
}