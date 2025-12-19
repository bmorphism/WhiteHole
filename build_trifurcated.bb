#!/usr/bin/env bb
;; WhiteHole Trifurcated Build Script
;; SplitMixTernary: MINUS(-1), ERGODIC(0), PLUS(+1)
;; Seed: 1069 = [+1, -1, -1, +1, +1, +1, +1]

(require '[babashka.process :refer [shell process]]
         '[babashka.fs :as fs]
         '[clojure.string :as str])

;; SplitMixTernary Constants
(def GOLDEN 0x9e3779b97f4a7c15)
(def MIX1 0xbf58476d1ce4e5b9)
(def MIX2 0x94d049bb133111eb)
(def SEED 1069)

;; All supported sample rates
(def ALL_SAMPLE_RATES [8000 16000 44100 48000 88200 96000 176400 192000 352800 384000 705600 768000])

;; Trifurcated device configurations
(def trifurcated-configs
  [{:trit :minus
    :name "WhiteHole69"
    :bundle-id "audio.existential.WhiteHole69"
    :icon "WhiteHoleMinus.icns"
    :channels 69
    :sample-rates [44100 48000 96000]
    :latency 64
    :color-routing true
    :description "Contracting: Realtime focus, minimal latency"}
   
   {:trit :ergodic
    :name "WhiteHole256"
    :bundle-id "audio.existential.WhiteHole256"
    :icon "WhiteHoleZero.icns"
    :channels 256
    :sample-rates [44100 48000 88200 96000 176400 192000]
    :latency 256
    :color-routing true
    :description "Ergodic: Balanced synthesis, standard workflow"}
   
   {:trit :plus
    :name "WhiteHole1069"
    :bundle-id "audio.existential.WhiteHole1069"
    :icon "WhiteHolePlus.icns"
    :channels 1069  ;; Seed as channel count - maximum bandwidth
    :sample-rates ALL_SAMPLE_RATES
    :latency 512
    :color-routing true
    :description "Expanding: Maximum bandwidth, all rates"}])

(defn trit->symbol [trit]
  (case trit
    :minus "🔴 MINUS (-1)"
    :ergodic "🟡 ERGODIC (0)"
    :plus "🟢 PLUS (+1)"))

(defn bandwidth [config]
  (let [max-rate (apply max (:sample-rates config))
        channels (:channels config)]
    (* channels max-rate 32))) ;; 32-bit float

(defn format-bandwidth [bw]
  (cond
    (>= bw 1e12) (format "%.2f Tbps" (/ bw 1e12))
    (>= bw 1e9) (format "%.2f Gbps" (/ bw 1e9))
    (>= bw 1e6) (format "%.2f Mbps" (/ bw 1e6))
    :else (format "%d bps" bw)))

(defn build-preprocessor-defs [config]
  (str/join " "
    [(format "kDriver_Name=\\\"%s\\\"" (:name config))
     (format "kPlugIn_BundleID=\\\"%s\\\"" (:bundle-id config))
     (format "kPlugIn_Icon=\\\"%s\\\"" (:icon config))
     (format "kNumber_Of_Channels=%d" (:channels config))
     (format "kSampleRates=%s" (str/join "," (:sample-rates config)))
     (format "kLatency_Frame_Size=%d" (:latency config))
     "kEnableColorRouting=1"
     (format "kBloomFilterSeed=%d" SEED)
     "kDefaultRejectMode=kRejectMode_Silent"
     "kDevice_IsHidden=false"
     "kDevice_HasInput=true"
     "kDevice_HasOutput=true"
     "kDevice2_IsHidden=false"
     "kDevice2_HasInput=true"
     "kDevice2_HasOutput=true"]))

(defn xcodebuild-cmd [config]
  (let [defs (build-preprocessor-defs config)]
    ["xcodebuild"
     "-project" "WhiteHole.xcodeproj"
     "-configuration" "Release"
     (str "PRODUCT_BUNDLE_IDENTIFIER=" (:bundle-id config))
     "CODE_SIGN_IDENTITY=" 
     "CODE_SIGNING_REQUIRED=NO"
     (str "GCC_PREPROCESSOR_DEFINITIONS=$GCC_PREPROCESSOR_DEFINITIONS " defs)]))

(defn install-driver! [config]
  (let [driver-name (str (:name config) ".driver")
        src-path (str "build/Release/" driver-name)
        dst-path (str "/Library/Audio/Plug-Ins/HAL/" driver-name)]
    (println (format "  Installing %s -> %s" driver-name dst-path))
    (shell {:dir "."} "sudo" "cp" "-R" src-path dst-path)))

(defn build-variant! [config]
  (println)
  (println (str "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"))
  (println (format "  %s: %s" (trit->symbol (:trit config)) (:name config)))
  (println (format "  %s" (:description config)))
  (println (format "  Channels: %d | Bandwidth: %s" 
                   (:channels config) 
                   (format-bandwidth (bandwidth config))))
  (println (str "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"))
  
  (let [cmd (xcodebuild-cmd config)]
    (println "  Building...")
    (apply shell {:dir "." :out :string :err :string} cmd)
    (println "  ✓ Build complete")))

(defn restart-coreaudio! []
  (println)
  (println "⚡ Restarting CoreAudio...")
  (shell "sudo" "killall" "-9" "coreaudiod")
  (println "✓ CoreAudio restarted"))

(defn print-banner []
  (println)
  (println "╔═══════════════════════════════════════════════════════════════╗")
  (println "║  WhiteHole Trifurcated Build System                           ║")
  (println "║  SplitMixTernary XOR-Independence Protocol                    ║")
  (println "║  Seed: 1069 = [+1, -1, -1, +1, +1, +1, +1]                     ║")
  (println "╚═══════════════════════════════════════════════════════════════╝"))

(defn print-summary []
  (println)
  (println "┌─────────────────────────────────────────────────────────────────┐")
  (println "│  Trifurcation Summary                                           │")
  (println "├─────────────────────────────────────────────────────────────────┤")
  (doseq [config trifurcated-configs]
    (println (format "│  %s %-20s %4d ch  %s │"
                     (case (:trit config) :minus "🔴" :ergodic "🟡" :plus "🟢")
                     (:name config)
                     (:channels config)
                     (format-bandwidth (bandwidth config)))))
  (println "└─────────────────────────────────────────────────────────────────┘")
  (println)
  (println "Total combined bandwidth:" 
           (format-bandwidth (reduce + (map bandwidth trifurcated-configs)))))

(defn -main [& args]
  (let [whitehole-dir (or (first args) ".")]
    (print-banner)
    
    (when-not (fs/exists? (fs/path whitehole-dir "WhiteHole.xcodeproj"))
      (println "ERROR: WhiteHole.xcodeproj not found in" whitehole-dir)
      (System/exit 1))
    
    ;; Trifurcate: build all three variants in sequence
    ;; (XOR-independent but sequential for resource management)
    (doseq [config trifurcated-configs]
      (build-variant! config))
    
    (print-summary)
    
    (println)
    (println "To install and restart CoreAudio, run:")
    (println "  sudo bb build_trifurcated.bb --install")
    
    (when (some #{"--install"} args)
      (doseq [config trifurcated-configs]
        (install-driver! config))
      (restart-coreaudio!))))

;; Entry point
(when (= *file* (System/getProperty "babashka.file"))
  (apply -main *command-line-args*))

;; REPL helpers
(comment
  ;; Build all variants
  (-main "/Users/barton/ies/WhiteHole")
  
  ;; Check bandwidth calculations
  (map (juxt :name bandwidth) trifurcated-configs)
  
  ;; Print xcodebuild command for a config
  (println (str/join " \\\n  " (xcodebuild-cmd (first trifurcated-configs))))
  
  ;; Verify seed 1069 in balanced ternary
  ;; 1069 = 3^6 + 3^4 + 3^3 + 3^2 + 3^1 + 3^0
  ;;      = 729 + 81 + 27 + 9 + 3 + 1 = 850 (wrong)
  ;; Actually: 1069 in balanced ternary = [+1 -1 -1 +1 +1 +1 +1]
  ;; +1*729 -1*243 -1*81 +1*27 +1*9 +1*3 +1*1 = 729-243-81+27+9+3+1 = 445 (wrong)
  ;; Need proper conversion
  )
