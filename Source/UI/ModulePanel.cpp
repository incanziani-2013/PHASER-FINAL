#include "ModulePanel.h"

namespace phaser::ui
{
    namespace
    {
        using SA = juce::AudioProcessorValueTreeState;

        class ChoiceRow : public juce::Component
        {
        public:
            ChoiceRow (SA& s, const juce::String& id, const juce::String& label, bool cy) : text (label), cyanAccent (cy)
            {
                auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id));
                jassert (p != nullptr);
                combo.addItemList (p->choices, 1);
                combo.setTooltip (p->getName (40));
                addAndMakeVisible (combo);
                att = std::make_unique<SA::ComboBoxAttachment> (s, id, combo);
            }
            void resized() override
            {
                auto b = getLocalBounds();
                const int h = juce::jmin (b.getHeight(), (int) (getWidth() * 0.16f));
                combo.setBounds (b.getX(), b.getCentreY() - h / 2 + (int) (h * 0.35f), b.getWidth(), h);
            }
            void paint (juce::Graphics& g) override
            {
                g.setColour (textCol); g.setFont (font ((float) getHeight() * 0.2f).withExtraKerningFactor (0.06f));
                g.drawText (text, 0, (int) (getHeight() * 0.12f), getWidth(), (int) (getHeight() * 0.28f), juce::Justification::centredLeft);
                juce::ignoreUnused (cyanAccent);
            }
        private:
            juce::String text; bool cyanAccent;
            juce::ComboBox combo;
            std::unique_ptr<SA::ComboBoxAttachment> att;   // se destruye antes que el combo
        };

        class ToggleRow : public juce::Component
        {
        public:
            ToggleRow (SA& s, const std::vector<std::pair<juce::String, juce::String>>& items, bool cy)
            {
                for (auto& it : items)
                {
                    auto b = std::make_unique<juce::TextButton> (it.second);
                    b->setClickingTogglesState (true);
                    b->getProperties().set ("cyan", cy);
                    if (auto* p = s.getParameter (it.first)) b->setTooltip (p->getName (40));
                    addAndMakeVisible (*b);
                    atts.push_back (std::make_unique<SA::ButtonAttachment> (s, it.first, *b));
                    buttons.push_back (std::move (b));
                }
            }
            ~ToggleRow() override { atts.clear(); }
            void resized() override
            {
                const int n = (int) buttons.size(); if (n == 0) return;
                const int gap = 4, w = (getWidth() - gap * (n - 1)) / n, h = juce::jmin (getHeight(), (int) (getWidth() * 0.12f));
                for (int i = 0; i < n; ++i) buttons[(size_t) i]->setBounds (i * (w + gap), getHeight() / 2 - h / 2, w, h);
            }
        private:
            std::vector<std::unique_ptr<juce::TextButton>> buttons;
            std::vector<std::unique_ptr<SA::ButtonAttachment>> atts;
        };
    }

    void ModulePanel::clear()
    {
        enableAtt.reset();
        rows.clear();   // los controles destruyen sus attachments antes que sus componentes
    }

    void ModulePanel::setContent (const juce::String& t, bool cy, const juce::String& enableId, const std::vector<Spec>& specs)
    {
        clear();
        title = t; cyanAccent = cy;

        if (enableId.isNotEmpty())
        {
            enableBtn.setClickingTogglesState (true);
            enableBtn.getProperties().set ("cyan", cy);
            enableBtn.setTooltip ("Activar / desactivar este módulo");
            addAndMakeVisible (enableBtn);
            enableAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, enableId, enableBtn);
        }
        else enableBtn.setVisible (false);

        for (auto& sp : specs)
        {
            std::unique_ptr<juce::Component> c;
            switch (sp.type)
            {
                case Spec::Type::Knob:    c = std::make_unique<KnobControl> (apvts, sp.id, sp.label, cy); break;
                case Spec::Type::Choice:  c = std::make_unique<ChoiceRow> (apvts, sp.id, sp.label, cy); break;
                case Spec::Type::Toggles: c = std::make_unique<ToggleRow> (apvts, sp.toggles, cy); break;
            }
            addAndMakeVisible (*c);
            rows.push_back (std::move (c));
        }
        resized(); repaint();
    }

    void ModulePanel::resized()
    {
        const float s = (float) getWidth() / 300.0f;
        const int headerH = (int) (58 * s), padX = (int) (26 * s), padB = (int) (22 * s);
        auto area = getLocalBounds().withTrimmedTop (headerH).withTrimmedBottom (padB).reduced (padX, 0);
        const int rowH = area.getHeight() / 4;
        for (size_t i = 0; i < rows.size(); ++i)
            rows[i]->setBounds (area.getX(), area.getY() + (int) i * rowH, area.getWidth(), rowH);

        if (enableBtn.isVisible())
        {
            const int bw = (int) (52 * s), bh = (int) (26 * s);
            const int bx = right ? (int) (14 * s) : getWidth() - (int) (66 * s);
            enableBtn.setBounds (bx, (int) (10 * s), bw, bh);
        }
    }

    void ModulePanel::paint (juce::Graphics& g)
    {
        const auto accent = cyanAccent ? cyan : green;
        const float s = (float) getWidth() / 300.0f;
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        auto frame = chamfered (r, 26.0f * s);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff06160e).withAlpha (0.92f), 0, r.getY(), juce::Colour (0xff020805).withAlpha (0.92f), 0, r.getBottom(), false));
        g.fillPath (frame);
        glowStroke (g, frame, accent.withAlpha (0.85f), 1.6f, 0.7f);

        auto inner = chamfered (r.reduced (10.0f * s).withTrimmedTop (44.0f * s), 18.0f * s);
        g.setColour (accent.withAlpha (0.22f)); g.strokePath (inner, juce::PathStrokeType (1.0f));

        // título + barras diagonales
        // Keep the module title and ON switch in separate header zones.
        g.setColour (accent); g.setFont (font (19.0f * s).withExtraKerningFactor (0.045f));
        const auto titleArea = juce::Rectangle<float> (right ? 76.0f * s : 28.0f * s,
                                                        7.0f * s,
                                                        right ? 208.0f * s : 190.0f * s,
                                                        30.0f * s);
        g.drawText (title, titleArea, right ? juce::Justification::centredRight : juce::Justification::centredLeft, true);

        const float sx = right ? 88.0f * s : (float) getWidth() - 122.0f * s;
        for (int i = 0; i < 4; ++i)
        {
            juce::Path p; const float x = sx + (float) i * 20.0f * s, y = 43.0f * s, w = 8.0f * s, hh = 8.0f * s;
            p.startNewSubPath (x + 5 * s, y); p.lineTo (x + 5 * s + w, y); p.lineTo (x + w, y + hh); p.lineTo (x, y + hh); p.closeSubPath();
            g.setColour (accent.withAlpha (0.35f + 0.15f * (float) i)); g.fillPath (p);
        }
    }
}
