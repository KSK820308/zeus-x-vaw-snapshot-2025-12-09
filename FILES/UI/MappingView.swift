import SwiftUI
import AVFoundation
import Combine
import AppKit

struct VelocityLayerUI: Identifiable, Hashable {
    let id = UUID()
    var min: Int
    var max: Int
    var sampleURL: URL
}

struct MappingZoneUI: Identifiable, Hashable {
    let id = UUID()
    var lowNote: Int
    var highNote: Int
    var rootKey: Int
    var tuneCents: Int
    var loops: [String: Int] = ["start": 0, "end": 0]
    var layers: [VelocityLayerUI]
}

struct MappingView: View {
    var midi: MIDIManager? = nil
    @State private var zones: [MappingZoneUI] = []
    @State private var selectedZoneID: UUID?
    @State private var anchorKey: Int = 60
    @State private var userSlot: Int = 1
    @State private var instrumentName: String = "NewInstrument"
    @State private var msb: Int = 63
    @State private var lsb: Int = 12
    @State private var pc: Int = 0
    @State private var status: String = ""
    @State private var lastActive: Set<Int> = []
    @State private var initializedBankCheck: Bool = false

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 12) {
                Picker("USER", selection: $userSlot) {
                    ForEach(1...20, id:\.self) { i in Text("USER\(i)").tag(i) }
                }.frame(width: 120)
                TextField("Instrument Name", text: $instrumentName).frame(width: 220)
                Stepper("Anchor: \(noteName(anchorKey))", value: $anchorKey, in: 0...127).frame(width: 180)
                HStack { Text("MSB:"); TextField("", value: $msb, formatter: NumberFormatter()) }.frame(width: 120)
                HStack { Text("LSB:"); TextField("", value: $lsb, formatter: NumberFormatter()) }.frame(width: 120)
                HStack { Text("PC:"); TextField("", value: $pc, formatter: NumberFormatter()) }.frame(width: 100)
                Spacer()
                Button("Export → ZXI") { exportZXI() }
            }
            .textFieldStyle(RoundedBorderTextFieldStyle())

            MappingHeader(selected: zones.first(where: { $0.id == selectedZoneID }))

            ScrollView(.horizontal) {
                VStack(spacing: 4) {
                    MappingCanvasView(zones: $zones, selectedZoneID: $selectedZoneID, onPlay: { note in previewNote(note) })
                        .frame(height: 220)
                        .onDrop(of: ["public.file-url"], isTargeted: nil) { providers in
                            handleDrop(providers: providers)
                        }
                    KeyboardView(selectedKey: $anchorKey, onPress: { _ in })
                        .frame(height: 60)
                }
            }

            if let sel = zones.first(where: { $0.id == selectedZoneID }) {
                ZoneEditor(zone: binding(for: sel))
            }

            HStack(spacing: 12) {
                Button("Delete Selected") { if let sel = selectedZoneID { zones.removeAll { $0.id == sel }; selectedZoneID = nil } }
                Button("Delete All") { zones.removeAll(); selectedZoneID = nil }
                Button("Nudge ◀︎") { nudgeSelection(by: -1) }
                Button("Nudge ▶︎") { nudgeSelection(by: 1) }
                Spacer()
                Text(status).foregroundColor(.green)
            }
        }
        .padding()
        .onReceive((midi != nil ? midi!.$activeNotes.eraseToAnyPublisher() : Just(Set<Int>()).eraseToAnyPublisher())) { notes in
            handleMidiActive(notes)
        }
        .onReceive((midi != nil ? midi!.$lastNoteOn.eraseToAnyPublisher() : Just(nil).eraseToAnyPublisher())) { evt in
            guard let evt else { return }
            handleMidiNoteOn(evt.0, velocity: evt.1)
        }
        .onReceive((midi != nil ? midi!.$lastNoteOff.eraseToAnyPublisher() : Just(nil).eraseToAnyPublisher())) { n in
            guard let n else { return }
            previewer.stopIfMatching(note: n)
        }
        .onAppear {
            // prefer MSB=63 / LSB=12; fill next free PC; if full, warn in English
            msb = 63; lsb = 12
            if let next = VoiceActions.nextFreePC(msb: msb, lsb: lsb) {
                pc = next
            } else {
                if !initializedBankCheck {
                    let alert = NSAlert()
                    alert.messageText = "USER Bank Full"
                    alert.informativeText = "MSB 63 / LSB 12 is full (0–127). Please select another LSB."
                    alert.alertStyle = .warning
                    alert.addButton(withTitle: "OK")
                    alert.runModal()
                    initializedBankCheck = true
                }
            }
        }
        .onChange(of: msb) { _ in updatePCForCurrentBank() }
        .onChange(of: lsb) { _ in updatePCForCurrentBank() }
    }

    private func binding(for zone: MappingZoneUI) -> Binding<MappingZoneUI> {
        guard let idx = zones.firstIndex(of: zone) else { return .constant(zone) }
        return $zones[idx]
    }

    private func handleDrop(providers: [NSItemProvider]) -> Bool {
        let group = DispatchGroup()
        var urls: [URL] = []
        for p in providers {
            if p.hasItemConformingToTypeIdentifier("public.file-url") {
                group.enter()
                p.loadItem(forTypeIdentifier: "public.file-url", options: nil) { item, _ in
                    if let data = item as? Data, let url = URL(dataRepresentation: data, relativeTo: nil) { urls.append(url) }
                    else if let str = item as? String, let url = URL(string: str) { urls.append(url) }
                    group.leave()
                }
            }
        }
        group.notify(queue: .main) {
            urls.sort { $0.lastPathComponent < $1.lastPathComponent }
            autoMap(urls: urls)
        }
        return true
    }

    private func autoMap(urls: [URL]) {
        guard !urls.isEmpty else { return }
        var key = anchorKey
        var newZones: [MappingZoneUI] = []
        for u in urls {
            let rk = parseNoteName(url: u) ?? key
            let z = MappingZoneUI(lowNote: rk, highNote: rk, rootKey: rk, tuneCents: 0, layers: [VelocityLayerUI(min: 1, max: 127, sampleURL: u)])
            newZones.append(z)
            key = min(127, rk + 1)
        }
        zones.append(contentsOf: newZones)
    }

    @StateObject private var previewer = MappingPreview()
    private func previewNote(_ note: Int) {
        guard let z = zones.first(where: { note >= $0.lowNote && note <= $0.highNote }) else { return }
        guard let lay = z.layers.first else { return }
        previewer.play(url: lay.sampleURL, note: note, rootKey: z.rootKey)
    }

    private func handleMidiNoteOn(_ note: Int, velocity: Int) {
        guard let z = zones.first(where: { note >= $0.lowNote && note <= $0.highNote }) else { return }
        let lay = z.layers.sorted { $0.min < $1.min }.last { velocity >= $0.min && velocity <= $0.max } ?? z.layers.first!
        previewer.play(url: lay.sampleURL, note: note, rootKey: z.rootKey)
        anchorKey = note
        selectedZoneID = z.id
    }

    private func handleMidiActive(_ notes: Set<Int>) {
        // find added notes
        let added = notes.subtracting(lastActive)
        let removed = lastActive.subtracting(notes)
        if let n = added.sorted().last { anchorKey = n; previewNote(n) }
        if notes.isEmpty && !removed.isEmpty { previewer.stop() }
        lastActive = notes
    }

    private func exportZXI() {
        // keep user-selected MSB/LSB; ensure PC is free within that bank
        if VoiceActions.findZXIByMSBLSBPC(msb: msb, lsb: lsb, pc: pc) != nil {
            if let next = VoiceActions.nextFreePC(msb: msb, lsb: lsb) {
                pc = next
            } else {
                let alert = NSAlert()
                alert.messageText = "USER Bank Full"
                alert.informativeText = "MSB \(msb) / LSB \(lsb) is full. Please select another LSB."
                alert.alertStyle = .warning
                alert.addButton(withTitle: "OK")
                alert.runModal()
                return
            }
        }
        let base = VoiceActions.projectUserPath().appendingPathComponent("USER\(userSlot)").appendingPathComponent(instrumentName).appendingPathComponent("Instrument.zxi")
        let samplesDir = base.appendingPathComponent("samples")
        let fm = FileManager.default
        try? fm.createDirectory(at: samplesDir, withIntermediateDirectories: true)
        var zxZones: [ZXIZone] = []
        for z in zones {
            var outLayers: [ZXIVelocityLayer] = []
            for lay in z.layers {
                let fn = noteName(z.rootKey) + "_vel\(lay.min)_\(lay.max).wav"
                let dest = samplesDir.appendingPathComponent(fn)
                try? fm.removeItem(at: dest)
                try? fm.copyItem(at: lay.sampleURL, to: dest)
                outLayers.append(ZXIVelocityLayer(min: lay.min, max: lay.max, samples: [fn]))
            }
            zxZones.append(ZXIZone(lowNote: z.lowNote, highNote: z.highNote, rootKey: z.rootKey, tuneCents: z.tuneCents, loops: z.loops, velocityLayers: outLayers))
        }
        let meta = ZXIInstrumentMeta(version: 1, name: instrumentName, msb: msb, lsb: lsb, pc: pc, zones: zxZones)
        let metaURL = base.appendingPathComponent("meta.json")
        if let data = try? JSONEncoder().encode(meta) { try? data.write(to: metaURL) }
        VoiceActions.updateLibraryIndex(msb: msb, lsb: lsb, pc: pc, path: base.path)
        status = "Готово: \(zones.count) файла в samples"
    }

    private func updatePCForCurrentBank() {
        if let next = VoiceActions.nextFreePC(msb: msb, lsb: lsb) { pc = next }
        else {
            let alert = NSAlert()
            alert.messageText = "USER Bank Full"
            alert.informativeText = "MSB \(msb) / LSB \(lsb) is full. Please select another LSB."
            alert.alertStyle = .warning
            alert.addButton(withTitle: "OK")
            alert.runModal()
        }
    }

    private func nudgeSelection(by semitones: Int) {
        if let sel = selectedZoneID, let idx = zones.firstIndex(where: { $0.id == sel }) {
            var z = zones[idx]
            z.lowNote = max(0, min(127, z.lowNote + semitones))
            z.highNote = max(0, min(127, z.highNote + semitones))
            z.rootKey = max(z.lowNote, min(z.highNote, z.rootKey + semitones))
            zones[idx] = z
        } else {
            zones = zones.map { var z = $0; z.lowNote = max(0, min(127, z.lowNote + semitones)); z.highNote = max(0, min(127, z.highNote + semitones)); z.rootKey = max(z.lowNote, min(z.highNote, z.rootKey + semitones)); return z }
        }
    }

    private func noteName(_ n: Int) -> String {
        let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        let octave = n/12 - 1
        return names[max(0, min(11, n%12))] + String(octave)
    }

    private func parseNoteName(url: URL) -> Int? {
        let name = url.deletingPathExtension().lastPathComponent.uppercased()
        let table: [(String,Int)] = [("C#",1),("DB",1),("D",2),("D#",3),("EB",3),("E",4),("F",5),("F#",6),("GB",6),("G",7),("G#",8),("AB",8),("A",9),("A#",10),("BB",10),("B",11),("C",0)]
        for (n, v) in table {
            if let r = name.range(of: n) {
                let tail = name[r.upperBound...]
                let num = tail.prefix { $0 == "-" || ($0 >= "0" && $0 <= "9") }
                if let o = Int(num) { return o*12 + v }
            }
        }
        return nil
    }
}

struct MappingHeader: View {
    let selected: MappingZoneUI?
    var body: some View {
        HStack(spacing: 16) {
            Text("Key Range:")
            Text(selected != nil ? "\(noteName(selected!.lowNote)) – \(noteName(selected!.highNote))" : "–")
            Text("Vel Range:")
            Text(selected != nil ? "\(selected!.layers.first?.min ?? 1) – \(selected!.layers.first?.max ?? 127)" : "–")
            Text("Root Key:")
            Text(selected != nil ? noteName(selected!.rootKey) : "–")
            Text("Volume:")
            Text("0.00 dB")
            Text("Pan:")
            Text("0.0")
            Text("Tune:")
            Text("0.00 st")
            Spacer()
        }
        .font(.caption)
        .padding(.horizontal, 4)
    }
    private func noteName(_ n: Int) -> String {
        let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        let octave = n/12 - 1
        return names[max(0, min(11, n%12))] + String(octave)
    }
}

struct MappingCanvasView: View {
    @Binding var zones: [MappingZoneUI]
    @Binding var selectedZoneID: UUID?
    var onPlay: (Int) -> Void
    private let keys = Array(0...127)

    var body: some View {
        let kw: CGFloat = 18
        let w = CGFloat(keys.count) * kw
        let h: CGFloat = 220
        ZStack(alignment: .topLeading) {
            HStack(spacing: 0) {
                ForEach(keys, id: \.self) { _ in
                    Rectangle().fill(Color.clear)
                        .frame(width: kw, height: h)
                        .overlay(Rectangle().stroke(Color.gray.opacity(0.25), lineWidth: 1))
                }
            }
            ForEach(zones) { z in
                let x = CGFloat(z.lowNote) * kw
                let width = CGFloat(z.highNote - z.lowNote + 1) * kw
                ZoneStripView(zone: binding(for: z), kw: kw)
                    .frame(width: width, height: h * 0.9)
                    .position(x: x + width/2, y: h*0.45)
                    .onTapGesture { selectedZoneID = z.id }
            }
        }
        .frame(width: w, height: h)
        .background(Color.gray.opacity(0.12))
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }

    private func binding(for zone: MappingZoneUI) -> Binding<MappingZoneUI> {
        guard let idx = zones.firstIndex(of: zone) else { return .constant(zone) }
        return $zones[idx]
    }

}

struct KeyboardView: View {
    @Binding var selectedKey: Int
    var onPress: (Int) -> Void
    private let keys = Array(0...127)
    var body: some View {
        let kw: CGFloat = 18
        let w = CGFloat(keys.count) * kw
        let h: CGFloat = 60
        ZStack(alignment: .bottomLeading) {
            HStack(spacing: 0) {
                ForEach(keys, id: \.self) { k in
                    Rectangle()
                        .fill(isBlack(k) ? Color.black : Color.white)
                        .overlay(Rectangle().stroke(Color.gray.opacity(0.4), lineWidth: 0.5))
                        .frame(width: kw, height: h)
                        .onTapGesture { selectedKey = k; onPress(k) }
                }
            }
            Rectangle()
                .fill(Color.blue.opacity(0.3))
                .frame(width: kw, height: h)
                .offset(x: CGFloat(selectedKey) * kw)
        }
        .frame(width: w, height: h)
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }
    private func isBlack(_ n: Int) -> Bool { [1,3,6,8,10].contains(n % 12) }
}

struct ZoneStripView: View {
    @Binding var zone: MappingZoneUI
    let kw: CGFloat
    @State private var dragState: DragState = .none

    enum DragState { case none, left, right, move }

    var body: some View {
        ZStack(alignment: .leading) {
            Rectangle().fill(Color.yellow.opacity(0.25)).overlay(Rectangle().stroke(Color.yellow, lineWidth: 1))
            HStack {
                Rectangle().fill(Color.yellow).frame(width: 4)
                    .gesture(DragGesture().onChanged { v in adjustLeft(dx: v.translation.width) })
                Spacer()
                Rectangle().fill(Color.yellow).frame(width: 4)
                    .gesture(DragGesture().onChanged { v in adjustRight(dx: v.translation.width) })
            }
        }
        .gesture(DragGesture().onChanged { v in move(dx: v.translation.width) })
        .overlay(Text(noteName(zone.rootKey)).font(.caption).padding(4), alignment: .topLeading)
        .overlay(VelGridOverlay(layers: zone.layers).padding(.top, 18))
    }

    private func adjustLeft(dx: CGFloat) { let d = Int(dx / kw); zone.lowNote = max(0, min(zone.highNote, zone.lowNote + d)) }
    private func adjustRight(dx: CGFloat) { let d = Int(dx / kw); zone.highNote = max(zone.lowNote, min(127, zone.highNote + d)) }
    private func move(dx: CGFloat) {
        let d = Int(dx / kw)
        let span = zone.highNote - zone.lowNote
        let nl = max(0, min(127 - span, zone.lowNote + d))
        zone.highNote = nl + span; zone.lowNote = nl; zone.rootKey = max(zone.lowNote, min(zone.highNote, zone.rootKey))
    }

    private func noteName(_ n: Int) -> String {
        let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        let octave = n/12 - 1
        return names[max(0, min(11, n%12))] + String(octave)
    }
}

struct VelGridOverlay: View {
    var layers: [VelocityLayerUI]
    var body: some View {
        GeometryReader { geo in
            let h = geo.size.height
            ZStack(alignment: .topLeading) {
                // grid lines
                ForEach(0..<12, id: \.self) { i in
                    Rectangle().fill(Color.gray.opacity(0.2)).frame(height: 1)
                        .offset(y: CGFloat(i) * (h/12))
                }
                // layers blocks
                ForEach(layers) { lay in
                    let y1 = h * (1 - CGFloat(lay.max)/127.0)
                    let y2 = h * (1 - CGFloat(lay.min)/127.0)
                    Rectangle().fill(Color.blue.opacity(0.2)).overlay(Rectangle().stroke(Color.blue, lineWidth: 1))
                        .frame(height: max(2, y2 - y1))
                        .offset(y: y1)
                }
            }
        }
    }
}

struct ZoneEditor: View {
    @Binding var zone: MappingZoneUI
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Zone Editor")
                .font(.headline)
            HStack {
                Stepper("Low: \(noteName(zone.lowNote))", value: $zone.lowNote, in: 0...zone.highNote)
                Stepper("High: \(noteName(zone.highNote))", value: $zone.highNote, in: zone.lowNote...127)
                Stepper("Root: \(noteName(zone.rootKey))", value: $zone.rootKey, in: zone.lowNote...zone.highNote)
                Spacer()
            }
            if let idx = zone.layers.indices.first {
                let layBinding = Binding(get: { zone.layers[idx] }, set: { zone.layers[idx] = $0 })
                HStack {
                    HStack { Text("Vel min"); Slider(value: Binding(get: { Double(layBinding.wrappedValue.min) }, set: { layBinding.wrappedValue.min = Int($0) }), in: 1...127) }
                    HStack { Text("Vel max"); Slider(value: Binding(get: { Double(layBinding.wrappedValue.max) }, set: { layBinding.wrappedValue.max = Int($0) }), in: 1...127) }
                }
                Text("Sample: \(layBinding.wrappedValue.sampleURL.lastPathComponent)").font(.caption)
            } else {
                Text("No velocity layer").font(.caption)
            }
        }
        .padding(8)
        .background(RoundedRectangle(cornerRadius: 6).fill(Color.gray.opacity(0.08)))
    }

    private func noteName(_ n: Int) -> String {
        let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        let octave = n/12 - 1
        return names[max(0, min(11, n%12))] + String(octave)
    }
}
