import SwiftUI
import CoreMIDI

struct TopBar: View {
    @ObservedObject var midi: MIDIManager
    @State private var tempo: Double = 120
    @State private var tempoText: String = "120"
    var body: some View {
        HStack(spacing: 16) {
            HStack(spacing: 8) {
                Text("Tempo")
                Button("-") {
                    tempo = max(40, tempo - 1)
                    tempoText = String(Int(tempo))
                }
                TextField("120", text: $tempoText)
                    .frame(width: 50)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .onSubmit {
                        if let v = Double(tempoText) { tempo = min(240, max(40, v)) }
                        tempoText = String(Int(tempo))
                    }
                Button("+") {
                    tempo = min(240, tempo + 1)
                    tempoText = String(Int(tempo))
                }
            }
            Divider().frame(height: 24)
            HStack(spacing: 8) {
                Text("MIDI")
                Picker("Source", selection: $midi.selectedSourceID) {
                    ForEach(midi.sources) { src in
                        Text(src.name).tag(src.id as MIDIUniqueID?)
                    }
                }
                .frame(width: 180)
            }
            Divider().frame(height: 24)
            HStack(spacing: 8) {
                Text("Chord:")
                Text(midi.detectedChord)
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundColor(Color.cyan)
            }
            Divider().frame(height: 24)
            HStack(spacing: 8) {
                Menu("Set Split") {
                    Button("GLOBAL") { midi.setAwaitSplitMode(.global) }
                    Divider()
                    Menu("RIGHT 1") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.right(1, .low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.right(1, .high)) }
                    }
                    Menu("RIGHT 2") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.right(2, .low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.right(2, .high)) }
                    }
                    Menu("RIGHT 3") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.right(3, .low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.right(3, .high)) }
                    }
                    Menu("RIGHT 4") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.right(4, .low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.right(4, .high)) }
                    }
                    Menu("RIGHT 5") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.right(5, .low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.right(5, .high)) }
                    }
                    Menu("LOWER") {
                        Button("Set Lowest") { midi.setAwaitSplitMode(.lower(.low)) }
                        Button("Set Highest") { midi.setAwaitSplitMode(.lower(.high)) }
                    }
                }
                if midi.awaitingSplitNote {
                    Text("Awaiting note…")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundColor(.yellow)
                }
                Text("Split:")
                Text(midi.noteName(for: midi.splitPoint))
                    .font(.system(size: 12, weight: .semibold))
            }
            Spacer()
        }
        .padding(8)
        .background(Color.black.opacity(0.12))
    }
}
