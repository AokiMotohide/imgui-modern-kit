// Compile and link fixture for the optional Show module's public API: every free function is referenced so a missing
// definition or a changed signature fails the build. It runs nothing.
#include <imkit/show.h>
#include <cstdio>
#include <type_traits>

int main() {
    using namespace imkit::show;
    // Controls.
    ValueEdit (*fader)(const char*, float*, FaderOptions, imkit::ComponentOptions) = &Fader;
    ValueEdit (*knob)(const char*, float*, KnobOptions, imkit::ComponentOptions) = &Knob;
    ValueEdit (*xy)(const char*, float*, float*, XYPadOptions, imkit::ComponentOptions) = &XYPad;
    PadEvent (*pads)(const char*, std::span<const PadView>, PadGridOptions, imkit::ComponentOptions) = &PadGrid;
    void (*meter)(const char*, std::span<const float>, std::span<const float>, MeterOptions, imkit::ComponentOptions) = &MeterBar;
    void (*timecode)(const char*, const TimecodeView&, imkit::ComponentOptions) = &TimecodeDisplay;
    BindingRequest (*badge)(const char*, const BindingInfo&, imkit::ComponentOptions) = &BindingBadge;
    bool (*hold)(const char*, HoldButtonOptions, imkit::ComponentOptions) = &HoldButton;
    bool (*arm)(const char*, bool*, imkit::ComponentOptions) = &ArmSwitch;
    void (*led)(const char*, LedState, const char*, imkit::ComponentOptions) = &StatusLed;
    // Live.
    CueRequest (*cues)(const char*, std::span<const CueView>, CueListState&, CueListOptions, imkit::ComponentOptions) = &CueList;
    GoBarRequest (*go)(const char*, const GoBarView&, bool*, imkit::ComponentOptions) = &GoBar;
    ClipLauncherRequest (*launcher)(const char*, const ClipLauncherView&, ClipLauncherOptions, imkit::ComponentOptions) = &ClipLauncher;
    MixerRequest (*mixer)(const char*, std::span<MixerStrip>, MixerState&, MixerOptions, imkit::ComponentOptions) = &Mixer;
    TransitionRequest (*transition)(const char*, const TransitionView&, imkit::ComponentOptions) = &TransitionBar;
    EffectRackRequest (*rack)(const char*, std::span<const EffectSlot>, EffectRackState&, imkit::ComponentOptions) = &EffectRack;
    ModulatorRequest (*modulator)(const char*, const ModulatorView&, imkit::ComponentOptions) = &ModulatorEditor;
    MappingRequest (*mappings)(const char*, std::span<const MappingRow>, MappingState&, imkit::ComponentOptions) = &MappingTable;
    MessageMonitorRequest (*monitor)(const char*, std::span<const MessageView>, MessageMonitorState&, ImVec2, imkit::ComponentOptions) = &MessageMonitor;
    int (*devices)(const char*, std::span<const DeviceView>, imkit::ComponentOptions) = &DeviceHealth;
    // Rules.
    void (*builder)(const char*, const RuleView&, const RuleContext&, RuleBuilderState&, RuleEditBuffer&, RuleBuilderOptions, imkit::ComponentOptions) = &RuleBuilder;
    RuleOutcome (*evaluate)(const RuleView&, const RuleContext&, VariableLookup, std::span<ConditionTrace>) = &EvaluateRule;
    std::size_t (*sentence)(const RuleView&, const RuleContext&, std::span<char>) = &FormatRuleSentence;
    std::size_t (*validate)(const RuleView&, const RuleContext&, std::span<RuleIssue>) = &ValidateRule;
    // Schedule.
    std::size_t (*occurrences)(const Recurrence&, std::int64_t, std::int64_t, TimeZone, std::int64_t, std::span<Occurrence>) = &NextOccurrences;
    bool (*cron)(std::string_view, CronSchedule&, std::span<char>) = &ParseCron;
    SolarTimes (*solar)(std::int32_t, double, double) = &SolarEvents;
    std::size_t (*conflicts)(std::span<const CalendarEvent>, std::span<ConflictPair>) = &FindConflicts;
    CalendarRequest (*calendar)(const char*, std::span<const CalendarEvent>, CalendarState&, CalendarOptions, imkit::ComponentOptions) = &Calendar;
    bool (*date)(const char*, std::int32_t*, DatePickerOptions, imkit::ComponentOptions, std::int32_t*) = &DatePicker;
    ValueEdit (*week)(const char*, std::bitset<336>*, WeekPainterOptions, imkit::ComponentOptions) = &WeekPainter;
    // Beat.
    BeatClockRequest (*clock)(const char*, const BeatClockView&, imkit::ComponentOptions) = &BeatClock;
    DeckRequest (*deck)(const char*, const DeckView&, float, imkit::ComponentOptions) = &Deck;
    WaveformRequest (*scroll)(const char*, const WaveformSpan&, ScrollingWaveformOptions, imkit::ComponentOptions) = &ScrollingWaveform;
    DjChannelRequest (*channel)(const char*, const DjChannel&, float, imkit::ComponentOptions) = &DjChannelStrip;
    void (*sequencer)(const char*, const SequencerView&, StepEditBuffer&, SequencerOptions, imkit::ComponentOptions) = &StepSequencer;
    // Script.
    ScriptPasteRequest (*paste)(const char*, char*, std::size_t, const ScriptAnalysis&, ScriptPasteState&, ScriptPasteOptions, imkit::ComponentOptions) = &ScriptPastePanel;
    ParamEdit (*params)(const char*, std::span<const ParamDecl>, std::span<ParamValue>, imkit::ComponentOptions) = &ParameterPanel;
    ImportRequest (*wizard)(const char*, ImportState&, std::span<const ImportColumn>, imkit::ComponentOptions) = &PasteImportWizard;
    char (*delimiter)(std::string_view) = &DetectDelimiter;

    static_assert(std::is_aggregate_v<ValueEdit> && std::is_trivially_copyable_v<ValueEdit>);
    static_assert(std::is_same_v<StableId, imkit::accessibility::StableId>);
    const void* all[] = {reinterpret_cast<const void*>(fader), reinterpret_cast<const void*>(knob), reinterpret_cast<const void*>(xy), reinterpret_cast<const void*>(pads),
                         reinterpret_cast<const void*>(meter), reinterpret_cast<const void*>(timecode), reinterpret_cast<const void*>(badge), reinterpret_cast<const void*>(hold),
                         reinterpret_cast<const void*>(arm), reinterpret_cast<const void*>(led), reinterpret_cast<const void*>(cues), reinterpret_cast<const void*>(go),
                         reinterpret_cast<const void*>(launcher), reinterpret_cast<const void*>(mixer), reinterpret_cast<const void*>(transition), reinterpret_cast<const void*>(rack),
                         reinterpret_cast<const void*>(modulator), reinterpret_cast<const void*>(mappings), reinterpret_cast<const void*>(monitor), reinterpret_cast<const void*>(devices),
                         reinterpret_cast<const void*>(builder), reinterpret_cast<const void*>(evaluate), reinterpret_cast<const void*>(sentence), reinterpret_cast<const void*>(validate),
                         reinterpret_cast<const void*>(occurrences), reinterpret_cast<const void*>(cron), reinterpret_cast<const void*>(solar), reinterpret_cast<const void*>(conflicts),
                         reinterpret_cast<const void*>(calendar), reinterpret_cast<const void*>(date), reinterpret_cast<const void*>(week), reinterpret_cast<const void*>(clock),
                         reinterpret_cast<const void*>(deck), reinterpret_cast<const void*>(scroll), reinterpret_cast<const void*>(channel), reinterpret_cast<const void*>(sequencer),
                         reinterpret_cast<const void*>(paste), reinterpret_cast<const void*>(params), reinterpret_cast<const void*>(wizard), reinterpret_cast<const void*>(delimiter)};
    for (const void* p : all)
        if (!p)
            return 1;
    std::puts("Show API compile fixture OK");
    return 0;
}
