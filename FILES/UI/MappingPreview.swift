import Foundation
import AVFoundation
import Combine

final class MappingPreview: ObservableObject {
    let objectWillChange = PassthroughSubject<Void, Never>()
    private let engine = AVAudioEngine()
    private let player = AVAudioPlayerNode()
    private let timePitch = AVAudioUnitTimePitch()
    private var currentFormat: AVAudioFormat?
    private var currentNote: Int? = nil
    private var cache: [URL: AVAudioPCMBuffer] = [:]
    private var formatForURL: [URL: AVAudioFormat] = [:]

    init() {
        engine.attach(player)
        engine.attach(timePitch)
        engine.connect(player, to: timePitch, format: nil)
        engine.connect(timePitch, to: engine.mainMixerNode, format: nil)
        engine.prepare()
        try? engine.start()
    }

    func play(url: URL, note: Int, rootKey: Int) {
        if !engine.isRunning { engine.prepare(); try? engine.start() }
        player.stop()
        let buf: AVAudioPCMBuffer
        let fmt: AVAudioFormat
        if let c = cache[url], let f = formatForURL[url] {
            buf = c; fmt = f
        } else {
            guard let file = try? AVAudioFile(forReading: url) else { return }
            fmt = file.processingFormat
            guard let nb = AVAudioPCMBuffer(pcmFormat: fmt, frameCapacity: AVAudioFrameCount(file.length)) else { return }
            try? file.read(into: nb)
            cache[url] = nb; formatForURL[url] = fmt
            buf = nb
        }
        if let cf = currentFormat {
            if cf.channelCount != fmt.channelCount {
                engine.pause(); engine.stop()
                engine.disconnectNodeInput(timePitch)
                engine.disconnectNodeOutput(player)
                engine.connect(player, to: timePitch, format: fmt)
                engine.connect(timePitch, to: engine.mainMixerNode, format: fmt)
                currentFormat = fmt
                engine.prepare(); try? engine.start()
            }
        } else {
            engine.pause(); engine.stop()
            engine.disconnectNodeInput(timePitch)
            engine.disconnectNodeOutput(player)
            engine.connect(player, to: timePitch, format: fmt)
            engine.connect(timePitch, to: engine.mainMixerNode, format: fmt)
            currentFormat = fmt
            engine.prepare(); try? engine.start()
        }
        let semitones = Float(note - rootKey)
        timePitch.pitch = semitones * 100.0
        player.scheduleBuffer(buf, at: nil, options: [], completionHandler: nil)
        player.play()
        currentNote = note
    }

    func stop() {
        player.stop()
    }

    deinit {
        player.stop(); engine.stop()
    }
    func stopIfMatching(note: Int) {
        if currentNote == note { player.stop(); currentNote = nil }
    }
}
