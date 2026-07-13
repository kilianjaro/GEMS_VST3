#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static const char* toucanSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" width="1019" height="650" viewBox="0 0 1019 650">
  <g>
    <path d="M982.56083,616.44467a4.48086,4.48086,0,0,1-.68339-.284c-.31106-.157-.606-.3463-.91986-.49689a.4411.4411,0,0,0-.31743.0226,10.40456,10.40456,0,0,0-2.08366,1.14869c-.72257.52152-1.47832.99809-2.23,1.478-.625.399-1.26257.77842-1.8998,1.15775-.1648.0981-.3687.13676-.52083.24867a1.47952,1.47952,0,0,1-1.37424.29838,6.23518,6.23518,0,0,1-1.69048-.86145,6.91585,6.91585,0,0,1-.69448-.51605c-.22421-.1999-.20392-.37376.04109-.53768a8.72361,8.72361,0,0,1,.78888-.48472c.81533-.42781,1.63827-.8411,2.45727-1.262.9985-.51309,2.01044-1.0023,2.98994-1.54949.9977-.55735,1.97043-1.16077,2.94055-1.76573,1.00822-.62874,2.00022-1.28348,3.04788-1.95812a2.47779,2.47779,0,0,1-.2371-.10782c-.31787-.19388-.64772-.37247-.94417-.59528-.14715-.11059-.23117-.30457.03222-.44669a2.11419,2.11419,0,0,1-1.17314-.644c-.28033-.33825-.40107-.50534.03218-.74861a1.4709,1.4709,0,0,0,.20653-.17834,1.18588,1.18588,0,0,0-.18182-.08664,2.49181,2.49181,0,0,1-1.58306-1.08178c-.1089-.17256-.08767-.252.0962-.31639.31916-.11184.63592-.23049.93935-.34106a7.77258,7.77258,0,0,1-.9008-.394,4.11143,4.11143,0,0,1-.81607-.62329,1.793,1.793,0,0,1-.28266-.55976c-.074-.168.0387-.22514.17452-.23885.25705-.02593.51506-.047.77317-.05674.24875-.00942.49813-.002.74724-.002a4.98019,4.98019,0,0,1-2.50846-1.31777c-.06672-.071-.06754-.20394-.09842-.30637-.23913.01048-.167-.25143-.26177-.37508.08042-.05731.16406-.16777.24075-.16314.43844.02644.87573.076,1.31242.12632.27852.03212.55531.07926.83288.11967a10.13421,10.13421,0,0,0-.99714-.61586,14.53852,14.53852,0,0,1-1.32958-.85747,2.19563,2.19563,0,0,1-.87138-1.11227c-.06687-.23449.02064-.33839.25545-.30167a12.36817,12.36817,0,0,1,3.16478,1.03385c.18809.08483.38079.15944.57136.23876l.03117-.08284a1.28076,1.28076,0,0,1-.20088-.10188c-.97711-.72942-1.96667-1.44326-2.92223-2.2a4.13305,4.13305,0,0,1-1.24276-1.51238,2.61933,2.61933,0,0,1-.11231-.67323c.20193.02882.40391.05724.60567.08716a.15726.15726,0,0,1,.05256.02327c.78127.45722,1.56359.91268,2.34085,1.37664.05352.03195.06905.12758.10347.196.191-.07727.46355.11692.48028.38695l.06014-.1452,1.73287,1.12605c-.01792-.26606-.40924-.26389-.31371-.52073a.89855.89855,0,0,1-.23658-.13341c-.5029-.5527-.99376-1.11638-1.49835-1.6675-.41685-.45528-.8882-.86695-1.26-1.35536a8.76686,8.76686,0,0,1-.83037-1.44389c-.07315-.148-.00646-.36512-.00386-.55057a2.33643,2.33643,0,0,1,.55424.15936c.2347.13471.42807.3396.65674.48717.19991.129.44617.19129.63377.33343.63434.4806,1.25416.98059,1.87124,1.48335.05364.04371.04643.16209.06738.24537.13436-.06246.15128-.06079.35519.14394.32748.32878.648.66447.97707.99166.16733.16638.34676.32059.52073.4803l.08254-.06954c-.39971-.56016-.81463-1.11019-1.19581-1.68269-.68576-1.02993-1.37055-2.06153-2.01644-3.1165a3.2869,3.2869,0,0,1-.54177-1.55464c-.0116-.23987.05877-.313.27631-.22215a.74919.74919,0,0,1,.17431.10269,20.21777,20.21777,0,0,1,2.67227,2.53085q1.05234,1.20228,2.10173,2.40716c.15691.17952.31688.35636.47545.53444l.06837-.06608c-.12014-.22408-.24222-.44716-.36009-.67244-.47223-.90254-.95015-1.80221-1.41114-2.71047a9.57183,9.57183,0,0,1-.49714-1.13906,12.663,12.663,0,0,1-.35223-1.26569c-.02027-.08448.0517-.19107.08114-.28747.08922.03332.203.04245.26357.104.32143.32683.65125.64932.938,1.00566.74822.92994,1.47657,1.87589,2.21065,2.81717.2714.348.543.69616.80329,1.05248.45483.62273.89929,1.253,1.35123,1.87785a13.73788,13.73788,0,0,0,.976,1.31766,4.27359,4.27359,0,0,1,1.24546,2.52706c.02128.23029.003.465.03412.69342.05116.37538.14471.74531.18853,1.12125.03629.31131.02555.628.03757.94226.0116.30323.02746.6063.03809.90956.00935.26674.00338.53069.03778.80021a5.17647,5.17647,0,0,0,.3341,1.39683c.63664-.52338,1.2655-.98787,1.83114-1.51961a7.69378,7.69378,0,0,1,1.96953-1.51541,4.13447,4.13447,0,0,1,2.47195-.436,4.56161,4.56161,0,0,0,.91939.04776c.9099-.02829,1.82084-.12854,2.72846-.10186a14.85187,14.85187,0,0,1,4.63308.96265,10.13976,10.13976,0,0,1,3.63172,2.19464,5.71491,5.71491,0,0,1,1.567,2.39644,1.04513,1.04513,0,0,1-.00759.35038c-.60613-.232-1.13846-.47129-1.69276-.63719a8.71584,8.71584,0,0,0-1.46842-.28972,17.85993,17.85993,0,0,0-2.06516-.154c-.78677-.00138-1.5732.092-2.36064.12046-.94.034-1.881.03779-2.82121.06559a2.5278,2.5278,0,0,0-.85171.09484,5.15437,5.15437,0,0,0-1.21013.66318c-.5651.43456-1.08856.92578-1.61013,1.41387-1.00682.94218-1.98535,1.91506-3.00577,2.842a11.5763,11.5763,0,0,1-1.4152,1.04048,4.52243,4.52243,0,0,1-1.40114.74344c-.37814.09328-.70137.40165-1.07895.50609a12.3329,12.3329,0,0,1-3.18105.414c-.29483.00283-.58971.00043-.8719.00043a2.09183,2.09183,0,0,1-2.85242,2.17032c-.1581-.04469-.36017-.07179-.29226-.29865.074-.24722.25965.01382.38452-.04909.00646-.00326.01872.00334.02765.00691a1.82412,1.82412,0,0,0,1.67785-.14882,1.65266,1.65266,0,0,0,.61152-.88c-.10274.06288-.17766.153-.27093.2159a2.66981,2.66981,0,0,1-.86192.53109,11.0441,11.0441,0,0,1-1.7689.088c-.08153.00131-.1648-.10533-.24726-.162.08494-.05552.16792-.15441.2552-.15837a4.24,4.24,0,0,1,.773.02987,1.88415,1.88415,0,0,0,1.751-.69221c-.08208-.05435-.12825-.06473-.15337-.04634a2.424,2.424,0,0,1-1.64691.40234c-.09083-.00534-.18414.03777-.27561.03509a1.1108,1.1108,0,0,1-.33367-.05566c-.0336-.01186-.06458-.15087-.04946-.161a.61468.61468,0,0,1,.26509-.11863c.44736-.04.89763-.05054,1.3438-.09956a.8518.8518,0,0,0,.693-.65015C983.35714,616.664,982.955,616.56712,982.56083,616.44467ZM995.0866,611.379c.49456-.4651,1.00766-.91225,1.53324-1.342.37446-.30618.7824-.57163,1.17837-.851a.83524.83524,0,0,1,.17063-.08086,11.33019,11.33019,0,0,1-.973-.85507,1.17315,1.17315,0,0,1-.28408-.66428c-.03855-.51324-.00893-1.03192.00017-1.54826.00318-.18052.02935-.36056.042-.541.02472-.35154-.10128-.47779-.44737-.48339a1.1119,1.1119,0,0,1-.29463-.08426c.01163.1527-.16519.17489-.33456.19978a3.21007,3.21007,0,0,0-1.78891,1.04449,1.7016,1.7016,0,0,0-.25481.66662c-.14427.44393-.29811.88584-.4161,1.33693-.1327.50729-.1965,1.03434-.3528,1.53321a6.96674,6.96674,0,0,0-.24256,1.42465c-.04963.50577-.0924,1.01381-.10051,1.52146a7.82766,7.82766,0,0,0,.09379.94477l.134.03822C993.52752,612.88452,994.29724,612.1213,995.0866,611.379Zm10.27451-3.86644.364-.12049-.163-.41495-.0663.03224.1804.35176-.36159-.02482a.59791.59791,0,0,1,.253-.53437,6.62692,6.62692,0,0,0,.64633-.69484c-.248-.10231-.41778-.17577-.59017-.2424-.14692-.05678-.27454-.3238-.40814-.104-.07042-.0678-.12515-.15955-.20345-.1893-.401-.15235-.80833-.28775-1.21257-.43157-.07241-.02576-.14066-.06324-.21623-.09778l-.0775.14023a.17708.17708,0,0,1-.04966-.01907c-.07646-.06433-.14144-.15928-.2291-.18975-.17048-.05924-.35507-.07674-.52983-.12571-.03595-.01007-.0545-.08222-.08848-.13789l-.66859.169.053-.22532-.76547.043.06981-.10653a4.40392,4.40392,0,0,1-.508-.07627,7.73291,7.73291,0,0,0-2.44657-.07112c-.192.00984-.47615-.12723-.57763.08542a5.13174,5.13174,0,0,0-.317,1.04846,1.42253,1.42253,0,0,0,.03.42575c.03016.33431.06081.66858.09285,1.02033l.30628.01659-.22646.1443a22.176,22.176,0,0,1,7.81449.56367Zm-7.389.255-.0443.0015-.05867-.20222c-.049.03612-.11543.06863-.11166.085.03151.13675.07894.26972.11539.40549a2.54864,2.54864,0,0,0,.06212.27765c.026.06587.08212.163.13244.16787a2.43331,2.43331,0,0,0,.42227-.025c-.02967.244.71751.40958,1.35253.27834l-.112-.17566c.03892-.0076.06958-.01923.07326-.01351.20773.32319.522.129.78554.15219a6.61767,6.61767,0,0,0,.95338.0021,3.35974,3.35974,0,0,0,.52876-.11533c.01241.01205.07693.12692.137.12464.44474-.01684.88932-.05,1.33213-.09567.08986-.00925.172-.09312.27129-.15042.02269.13224.13425.11979.32609.115a18.78052,18.78052,0,0,1,1.88849.06413c.26708.02034.51812.07842.74449-.13562l-.15449.1901,1.54771.27346c-.30572-.37665-.86855-.1624-1.07117-.5886l-1.20653-.19763.09764-.14341-.29546.12714.01681-.05435-.92571-.12184.004.04227-.44459-.16025c-.18808-.02654-.40272-.05089-.615-.08841-.2208-.039-.43886-.09334-.659-.13626-.26874-.05238-.53753-.1056-.808-.14781-.25974-.04053-.52082-.07883-.78277-.09724-.26869-.01888-.53937-.00728-.80878-.01859-.2778-.01165-.55512-.03482-.85806-.05461l-.04051.20628-.03887.00147-.03334-.2116-1.70349.139ZM972.6749,618.17968l-.52623.29348.00814.039.48382-.06914Zm33.669-11.91237-.03937.00773a2.13622,2.13622,0,0,0,.18929.357c.02576.0334.14064-.002.201-.00513Zm-23.22555,1.76507c-.405-.08934-.59394-.02-.62031.15929Zm2.16583-6.57718.02209.04038.47221-.28988-.03911-.059ZM989.1544,612.366l.05025.041.29758-.34221-.05675-.04434Zm-15.79031,5.38769.0275.0872.41652-.12869-.0278-.088Zm13.17238-15.90344-.03235-.04179-.3704.29192.054.061Zm-1.48436,6.03575h-.26424v.27325l.03368.00834.07376-.24731.06629-.01062.07688.20219.02777-.02319Zm-2.68523.18456-.30388.0278.1909.26085Zm8.8943,5.51-.01526.05726.48646.13227.01133-.0651Zm-15.56227,3.309-.03265-.0685-.424.20113.03626.076Zm4.44407-9.27584.05029-.005-.02873-.6578-.06339.00955Zm9.76538.82134-.03751-.07606-.37623.20637.02676.049Zm-7.96723-9.11715c-.11705-.10808-.40613-.08159-.20467-.34529C981.53356,599.24689,981.68821,599.29524,981.94113,599.31792Zm-.69624,10.16531.06134-.02862-.17708-.42218-.0601.02859Zm-2.86454-9.85723-.02634.044.30389.18066.02992-.05051Zm28.654,8.00178-.06077-.04584-.18985.32166.05734.03924Zm-30.41369,8.52544-.19371.11639.1731.262Zm-.33539.24868-.00749-.03419-.37585.12884.02221.09185Zm-2.303,1.166.05448.07174.28521-.20979-.04943-.06125Zm.99159.00645c.26307.0956.27366.09184.28169-.11267ZM983.962,604.657l-.00991-.04624-.41217.08429.01737.05711Zm-2.36534,4.30123-.0724.02754.12407.33267.078-.03Zm6.7815,1.89443-.06354.035.18995.33.05519-.03107Zm-15.58844,8.55278-.0269.04578.29264.15917.04753-.05086Z" fill="#e8e6de"/>
    <path d="M995.65249,605.66271a.54615.54615,0,0,0-.54759.51185.55725.55725,0,1,0,1.11442.00941A.54566.54566,0,0,0,995.65249,605.66271Z" fill="#e8e6de"/>
    <path d="M1005.64675,606.49465l-.518-.21226.26986.30111Z" fill="#e8e6de"/>
    <path d="M1000.84827,606.25218l-.09815-.35024-.07551.02325.09344.35295Z" fill="#e8e6de"/>
  </g>
</svg>
)SVG";

//==============================================================================
static void setupKnob (juce::Slider& s) { s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle (juce::Slider::NoTextBox,false,0,0); s.setMouseDragSensitivity (180); }
static const HemingLookAndFeel* gemsLnf (const juce::Component& c) { return dynamic_cast<const HemingLookAndFeel*> (&c.getLookAndFeel()); }

static const char* kNoteNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };

juce::String NewProjectAudioProcessorEditor::fmtPercent01(float v) { return juce::String ((int) std::round (juce::jlimit(0.f,1.f,v)*100.f)) + "%"; }
juce::String NewProjectAudioProcessorEditor::fmtHz(float hz) { hz=juce::jlimit(20.f,20000.f,hz); return (hz>=1000.f)?juce::String(hz/1000.f,1)+"K":juce::String((int)std::round(hz))+"HZ"; }
juce::String NewProjectAudioProcessorEditor::fmtInt(float v) { return juce::String((int)std::round(v)); }
juce::String NewProjectAudioProcessorEditor::fmtNote(int i) { return kNoteNames[juce::jlimit(0,11,i)]; }
juce::String NewProjectAudioProcessorEditor::fmtGatePercent(float d) { return juce::String((int)std::round(juce::jlimit(0.f,1.f,(d+42.f)/42.f)*100.f))+"%"; }
juce::String NewProjectAudioProcessorEditor::fmtSeconds(float s) { if(s<0.01f)return"0MS"; if(s<1.f)return juce::String((int)std::round(s*1000.f))+"MS"; return juce::String(s,2)+"S"; }
float NewProjectAudioProcessorEditor::readParam(const juce::String& id) const { auto*p=audioProcessor.apvts.getRawParameterValue(id); return p?p->load():0.f; }

//==============================================================================
//  Button paints
//==============================================================================
void BandSelectButton::paintButton (juce::Graphics& g, bool hl, bool)
{
    auto b = getLocalBounds().toFloat();
    const bool sel   = getToggleState();
    const bool multi = multiSelected;
    const auto ink   = lnf ? lnf->theme().ink : juce::Colour (0xffe8e6de);
    const auto carbon= lnf ? lnf->theme().carbon : juce::Colour (0xff0b0c0b);
    const auto col   = kBandColours[juce::jlimit (0, 7, bandIndex)];

    if (sel)
    {
        g.setColour (ink.withAlpha (0.94f));
        g.fillRect (b);
    }

    // outline
    if (sel)          g.setColour (ink);
    else if (multi)   g.setColour (col.withAlpha (0.85f));
    else              g.setColour (ink.withAlpha (hl ? 0.55f : 0.22f));
    g.drawRect (b, 1.0f);

    // voice colour tick — left edge
    g.setColour (col.withAlpha (sel || multi ? 1.0f : 0.55f));
    g.fillRect (b.getX() + (sel ? 3.0f : 2.0f), b.getY() + (sel ? 3.0f : 2.0f),
                3.0f, b.getHeight() - (sel ? 6.0f : 4.0f));

    // label
    g.setColour (sel ? carbon : (multi ? col : ink.withAlpha (hl ? 0.95f : 0.7f)));
    if (lnf) g.setFont (lnf->monoBold (10.0f));
    g.drawText ("B" + juce::String (bandIndex + 1),
                b.reduced (9.0f, 0.0f).toNearestInt().withTrimmedRight (10),
                juce::Justification::centred, false);
}

void BandLampButton::paintButton (juce::Graphics& g, bool hl, bool)
{
    auto b = getLocalBounds().toFloat();
    const auto col = kBandColours[juce::jlimit (0, 7, bandIndex)];
    auto box = juce::Rectangle<float> (0, 0, 7, 7).withCentre (b.getCentre());

    if (getToggleState())
    {
        g.setColour (col);
        g.fillRect (box);
    }
    g.setColour (getToggleState() ? col : juce::Colour (0xffe8e6de).withAlpha (hl ? 0.7f : 0.3f));
    g.drawRect (box, 1.0f);
}

void ScaleModeButton::paintButton (juce::Graphics& g, bool hl, bool)
{
    auto b = getLocalBounds().toFloat();
    const auto ink    = lnf ? lnf->theme().ink : juce::Colour (0xffe8e6de);
    const auto carbon = lnf ? lnf->theme().carbon : juce::Colour (0xff0b0c0b);
    const bool minor  = getToggleState();

    auto maj = b.removeFromLeft (b.getWidth() * 0.5f);
    auto& active   = minor ? b : maj;
    auto& inactive = minor ? maj : b;

    g.setColour (ink.withAlpha (0.92f));
    g.fillRect (active);
    g.setColour (ink.withAlpha (hl ? 0.6f : 0.3f));
    g.drawRect (getLocalBounds().toFloat(), 1.0f);

    if (lnf) g.setFont (lnf->mono (7.0f, 0.05f));
    g.setColour (carbon);
    g.drawText (minor ? "MIN" : "MAJ", active.toNearestInt(), juce::Justification::centred, false);
    g.setColour (ink.withAlpha (hl ? 0.6f : 0.38f));
    g.drawText (minor ? "MAJ" : "MIN", inactive.toNearestInt(), juce::Justification::centred, false);
}

void QuantizerNoteButton::paintButton (juce::Graphics& g, bool hl, bool)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    const auto ink    = lnf ? lnf->theme().ink : juce::Colour (0xffe8e6de);
    const auto carbon = lnf ? lnf->theme().carbon : juce::Colour (0xff0b0c0b);
    const bool on = getToggleState();

    auto cell = b.withTrimmedBottom (3.0f);

    if (on)
    {
        g.setColour (ink.withAlpha (hl ? 1.0f : 0.9f));
        g.fillRect (cell);
    }
    g.setColour (ink.withAlpha (on ? 0.95f : (hl ? 0.6f : 0.25f)));
    g.drawRect (cell, 1.0f);

    if (lnf) g.setFont (lnf->mono (6.8f));
    g.setColour (on ? carbon : ink.withAlpha (hl ? 0.8f : 0.45f));
    g.drawText (kNoteNames[juce::jlimit (0, 11, pitchClass)], cell.toNearestInt(),
                juce::Justification::centred, false);

    // quantizer activity — flash bar under the cell
    if (glow > 0.02f)
    {
        g.setColour (ink.withAlpha (glow));
        g.fillRect (b.getX(), b.getBottom() - 2.0f, b.getWidth(), 2.0f);
    }
}

void WaveformButton::paintButton(juce::Graphics& g,bool hl,bool){
    auto b=getLocalBounds().toFloat(); g.setColour(juce::Colour(0xffe8e6de).withAlpha(hl?.95f:.80f));
    auto ic=b.removeFromTop(b.getHeight()*.55f); ic=ic.reduced(ic.getWidth()*.15f,ic.getHeight()*.15f); drawWaveIcon(g,ic,currentWave);
    static const char*nm[]={"SINE","TRI","SAW","SQR"}; g.drawText(nm[juce::jlimit(0,3,currentWave)],b.toNearestInt(),juce::Justification::centred,false); }
void WaveformButton::drawWaveIcon(juce::Graphics& g,juce::Rectangle<float> r,int w){
    const float x0=r.getX(),x1=r.getRight(),ym=r.getCentreY(),a=r.getHeight()*.42f,ww=r.getWidth(); juce::Path p;
    switch(w){ case 0:p.startNewSubPath(x0,ym);p.cubicTo(x0+ww*.25f,ym-a*2,x0+ww*.25f,ym-a*2,x0+ww*.5f,ym);p.cubicTo(x0+ww*.75f,ym+a*2,x0+ww*.75f,ym+a*2,x1,ym);break;
    case 1:p.startNewSubPath(x0,ym);p.lineTo(x0+ww*.25f,ym-a);p.lineTo(x0+ww*.75f,ym+a);p.lineTo(x1,ym);break;
    case 2:p.startNewSubPath(x0,ym+a);p.lineTo(x0+ww*.5f,ym-a);p.lineTo(x0+ww*.5f,ym+a);p.lineTo(x1,ym-a);break;
    case 3:p.startNewSubPath(x0,ym+a);p.lineTo(x0,ym-a);p.lineTo(x0+ww*.5f,ym-a);p.lineTo(x0+ww*.5f,ym+a);p.lineTo(x1,ym+a);p.lineTo(x1,ym-a);break; }
    g.strokePath(p,juce::PathStrokeType(1.f)); }

//==============================================================================
//  OctaveControl
//==============================================================================
OctaveControl::OctaveControl(){ internalSlider.setSliderStyle(juce::Slider::LinearBarVertical); internalSlider.setTextBoxStyle(juce::Slider::NoTextBox,true,0,0); internalSlider.setRange(-4,4,1); internalSlider.setVisible(false); addChildComponent(internalSlider); }
void OctaveControl::paint (juce::Graphics& g)
{
    const auto ink = lnf ? lnf->theme().ink : juce::Colour (0xffe8e6de);
    const int v = getValue();

    if (lnf) g.setFont (lnf->monoBold (14.0f));
    g.setColour (v == 0 ? ink.withAlpha (0.55f) : accent.withAlpha (0.95f));
    g.drawText ((v > 0 ? "+" : "") + juce::String (v),
                0, 0, getWidth(), getHeight() - 14, juce::Justification::centred, false);

    if (lnf) g.setFont (lnf->grot (8.5f, 0.10f));
    g.setColour (ink.withAlpha (0.52f));
    g.drawText ("OCT", 0, getHeight() - 12, getWidth(), 10, juce::Justification::centred, false);

    // drag affordance ticks
    g.setColour (ink.withAlpha (0.3f));
    const float cx = (float) getWidth() - 5.0f, cy = (float) (getHeight() - 14) * 0.5f;
    juce::Path p;
    p.addTriangle (cx - 2.5f, cy - 4.0f, cx + 2.5f, cy - 4.0f, cx, cy - 8.0f);
    p.addTriangle (cx - 2.5f, cy + 4.0f, cx + 2.5f, cy + 4.0f, cx, cy + 8.0f);
    g.fillPath (p);
}
void OctaveControl::mouseDown(const juce::MouseEvent& e){ dragStartY=e.position.y; dragStartVal=(float)internalSlider.getValue(); }
void OctaveControl::mouseDrag(const juce::MouseEvent& e){ float dy=dragStartY-e.position.y; float ps=(float)getHeight()/2.25f;
    internalSlider.setValue(juce::jlimit(-4.0,4.0,(double)std::round(dragStartVal+dy/ps)),juce::sendNotificationSync); }
void OctaveControl::mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& w){ wheelAccum+=w.deltaY; const float th=.8f;
    if(std::abs(wheelAccum)>=th){int s=(int)(wheelAccum/th); internalSlider.setValue(juce::jlimit(-4.0,4.0,internalSlider.getValue()+s),juce::sendNotificationSync); wheelAccum-=s*th;} }
void OctaveControl::attachToParam(juce::AudioProcessorValueTreeState& a,const juce::String& id){ att.reset(); att=std::make_unique<SliderAttachment>(a,id,internalSlider); }
void OctaveControl::detachParam(){ att.reset(); }
int OctaveControl::getValue() const { return (int)std::round(internalSlider.getValue()); }

//==============================================================================
//  EngineSelector — [<] NAME [>] stepper cell
//==============================================================================
void EngineSelector::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto ink = lnfPtr ? lnfPtr->theme().ink : juce::Colour (0xffe8e6de);
    const bool hover = isMouseOver();
    const auto mouse = getMouseXYRelative();
    const bool overL = hover && mouse.x < kArrowW;
    const bool overR = hover && mouse.x > getWidth() - kArrowW;
    const bool overC = hover && ! overL && ! overR;

    g.setColour (ink.withAlpha (hover ? 0.55f : 0.3f));
    g.drawRect (r, 1.0f);
    g.drawLine ((float) kArrowW, r.getY() + 1, (float) kArrowW, r.getBottom() - 1, 1.0f);
    g.drawLine (r.getRight() - kArrowW, r.getY() + 1, r.getRight() - kArrowW, r.getBottom() - 1, 1.0f);

    if (lnfPtr) g.setFont (lnfPtr->mono (9.0f));
    g.setColour (ink.withAlpha (overL ? 1.0f : 0.5f));
    g.drawText ("<", juce::Rectangle<float> (0, 0, (float) kArrowW, r.getHeight()), juce::Justification::centred, false);
    g.setColour (ink.withAlpha (overR ? 1.0f : 0.5f));
    g.drawText (">", juce::Rectangle<float> (r.getRight() - kArrowW, 0, (float) kArrowW, r.getHeight()), juce::Justification::centred, false);

    auto mid = r.reduced ((float) kArrowW, 0.0f);

    // voice colour tick
    g.setColour (accent);
    g.fillRect (mid.getX() + 6.0f, mid.getCentreY() - 4.0f, 3.0f, 8.0f);

    if (lnfPtr) g.setFont (lnfPtr->monoBold (9.0f, 0.08f));
    g.setColour (ink.withAlpha (overC ? 1.0f : 0.88f));
    g.drawText (voiceEngineName (current), mid.toNearestInt(), juce::Justification::centred, false);
}

void EngineSelector::mouseDown (const juce::MouseEvent& e)
{
    const int wdt = getWidth();
    if (e.x < kArrowW)          { if (onPick) onPick ((current + kNumEngines - 1) % kNumEngines); return; }
    if (e.x > wdt - kArrowW)    { if (onPick) onPick ((current + 1) % kNumEngines); return; }

    juce::PopupMenu m;
    if (lnfPtr) m.setLookAndFeel (lnfPtr);
    for (int i = 0; i < kNumEngines; ++i)
        m.addItem (i + 1, voiceEngineName (i), true, i == current);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [this] (int res) { if (res > 0 && onPick) onPick (res - 1); });
}

void EngineSelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    wheelAccum += w.deltaY;
    if (std::abs (wheelAccum) >= 0.8f)
    {
        int step = (wheelAccum > 0) ? 1 : -1;
        wheelAccum = 0.0f;
        if (onPick) onPick ((current + step + kNumEngines) % kNumEngines);
    }
}

//==============================================================================
//  MixerComponent — 8 voice strips + dB ruler
//==============================================================================
MixerComponent::MixerComponent(){ for(int i=0;i<8;++i){ volSliders[i].setSliderStyle(juce::Slider::LinearVertical); volSliders[i].setTextBoxStyle(juce::Slider::NoTextBox,true,0,0); volSliders[i].setRange(0,2,.01); volSliders[i].setValue(1); addAndMakeVisible(volSliders[i]); } }
void MixerComponent::attachVolumes(juce::AudioProcessorValueTreeState& a){ for(int i=0;i<8;++i) volAtts[i]=std::make_unique<SliderAttachment>(a,bandVolId(i),volSliders[i]); }
void MixerComponent::setLevels(int b,float L,float R){ if(b>=0&&b<8){meterL[b]=L;meterR[b]=R;} }
void MixerComponent::setBandOn(int b,bool on){ if(b>=0&&b<8)bandOn[b]=on; }
float MixerComponent::gainToPosition(float gain){
    // dB scale: -48dB at bottom, 0dB near top, +6dB at very top
    if (gain < 1e-6f) return 0.0f;
    float dB = 20.0f * std::log10 (gain);
    return juce::jlimit (0.0f, 1.0f, (dB + 48.0f) / 54.0f); // -48→0, 0dB→0.889, +6→1.0
}
void MixerComponent::resized()
{
    const float W = (float) getWidth();
    const float sw = (W - (float) kRulerW) / 8.0f;
    for (int i = 0; i < 8; ++i)
    {
        juce::Rectangle<float> s ((float) kRulerW + sw * (float) i, (float) kHeadH,
                                  sw, (float) (getHeight() - kHeadH - kFootH));
        volSliders[i].setBounds (s.reduced (2.0f, 0.0f).toNearestInt());
    }
}
void MixerComponent::paint (juce::Graphics& g)
{
    const auto ink  = lnf ? lnf->theme().ink  : juce::Colour (0xffe8e6de);
    const auto well = lnf ? lnf->theme().well : juce::Colour (0xff111211);
    const float W = (float) getWidth(), H = (float) getHeight();
    const float sw = (W - (float) kRulerW) / 8.0f;
    const float fTop = (float) kHeadH, fBot = H - (float) kFootH, fh = fBot - fTop;

    // dB ruler
    {
        if (lnf) g.setFont (lnf->mono (6.0f));
        const float dbs[] = { 6.0f, 0.0f, -12.0f, -24.0f, -36.0f, -48.0f };
        for (float dB : dbs)
        {
            const float y = fBot - fh * (dB + 48.0f) / 54.0f;
            const bool zero = (dB == 0.0f);
            g.setColour (ink.withAlpha (zero ? 0.55f : 0.3f));
            g.fillRect ((float) kRulerW - 3.0f, y - 0.5f, 3.0f, 1.0f);
            g.setColour (ink.withAlpha (zero ? 0.5f : 0.28f));
            g.drawText (dB > 0 ? "+" + juce::String ((int) dB) : juce::String ((int) dB),
                        0, (int) y - 4, kRulerW - 5, 8, juce::Justification::centredRight, false);
        }
    }

    // 0 dB line across all strips
    {
        const float zeroY = fBot - fh * 48.0f / 54.0f;
        g.setColour (ink.withAlpha (0.12f));
        g.fillRect ((float) kRulerW, zeroY - 0.5f, W - (float) kRulerW, 1.0f);
    }

    for (int i = 0; i < 8; ++i)
    {
        juce::Rectangle<float> s ((float) kRulerW + sw * (float) i, 0.0f, sw, H);
        const auto col = kBandColours[i];
        const bool on = bandOn[i];

        // colour chip
        g.setColour (col.withAlpha (on ? 1.0f : 0.18f));
        g.fillRect (s.getX() + 2.0f, 0.0f, sw - 4.0f, 3.0f);

        // band number
        if (lnf) g.setFont (lnf->mono (6.5f));
        g.setColour (ink.withAlpha (on ? 0.7f : 0.25f));
        g.drawText (juce::String (i + 1), s.withY (4.0f).withHeight (9.0f).toNearestInt(),
                    juce::Justification::centred, false);

        // meters (behind fader)
        auto ma = juce::Rectangle<float> (s.getX() + 3.0f, fTop, sw - 6.0f, fh);
        const float gap = 1.0f, hw = (ma.getWidth() - gap) * 0.5f;
        auto mL = ma.withWidth (hw);
        auto mR = ma.withX (ma.getX() + hw + gap).withWidth (hw);
        g.setColour (well.brighter (0.02f));
        g.fillRect (mL); g.fillRect (mR);

        const auto mcol = col.withAlpha (0.8f);
        float pL = gainToPosition (meterL[i]);
        if (pL > 0.001f) { auto f = mL; f.removeFromTop (mL.getHeight() * (1 - pL)); g.setColour (mcol); g.fillRect (f); }
        float pR = gainToPosition (meterR[i]);
        if (pR > 0.001f) { auto f = mR; f.removeFromTop (mR.getHeight() * (1 - pR)); g.setColour (mcol); g.fillRect (f); }

        // fader dB readout
        float vol = (float) volSliders[i].getValue();
        int dv = (vol > .001f) ? (int) std::round (20.f * std::log10 (vol)) : -60;
        if (lnf) g.setFont (lnf->mono (6.0f));
        g.setColour (ink.withAlpha (on ? 0.5f : 0.2f));
        g.drawText (juce::String (dv), s.withY (fBot + 2.0f).withHeight (8.0f).toNearestInt(),
                    juce::Justification::centred, false);
    }
}

//==============================================================================
//  (1) ANALYSIS FIELD
//==============================================================================
float SpectrumAnalyzerComponent::hzToX (float hz, float w)
{
    const float minF = 20.0f, maxF = 20000.0f;
    float t = std::log (hz / minF) / std::log (maxF / minF);
    t = t * 0.85f + 0.15f * (3.0f * t * t - 2.0f * t * t * t);
    return juce::jlimit (0.0f, 1.0f, t) * w;
}

float SpectrumAnalyzerComponent::dbToY (float dB, float h)
{
    const float minDb = -48.0f, maxDb = 6.0f;
    return (1.0f - (dB - minDb) / (maxDb - minDb)) * h;
}

void SpectrumAnalyzerComponent::computeFilterResponse (int band)
{
    if (sampleRate <= 0.0) return;
    const float freq = bandFreq[band];
    const float res  = bandRes[band];
    const int   poles = (int) bandPoles[band];
    const float Q = 0.5f * std::pow (50.0f, res);
    const int   sections = BandpassBank::polesToSections (poles);
    float gainComp = (sections > 1) ? 1.0f / std::pow (juce::jmax (Q, 0.5f), (float)(sections - 1)) : 1.0f;
    gainComp = juce::jlimit (0.02f, 1.0f, gainComp);

    const float minF = 20.0f, maxF = 20000.0f;
    for (int i = 0; i < kResponsePts; ++i)
    {
        float t = (float) i / (float)(kResponsePts - 1);
        float hz = minF * std::pow (maxF / minF, t);

        const double w0 = juce::MathConstants<double>::twoPi * (double) freq / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * (double) Q);
        const double cosw0 = std::cos (w0);
        const double b0 = alpha;
        const double a0 = 1.0 + alpha;
        const double a1 = -2.0 * cosw0;
        const double a2 = 1.0 - alpha;

        const double w = juce::MathConstants<double>::twoPi * (double) hz / sampleRate;
        const double cosw = std::cos (w);
        const double sinw = std::sin (w);

        const double z1re = cosw, z1im = -sinw;
        const double z2re = 2.0*cosw*cosw - 1.0, z2im = -2.0*cosw*sinw;

        const double numRe = b0 + (-b0) * z2re;
        const double numIm = (-b0) * z2im;

        const double denRe = a0 + a1 * z1re + a2 * z2re;
        const double denIm = a1 * z1im + a2 * z2im;

        double magSq = (numRe*numRe + numIm*numIm) / juce::jmax (1.0e-20, denRe*denRe + denIm*denIm);

        double totalMagSq = 1.0;
        for (int s = 0; s < sections; ++s)
            totalMagSq *= magSq;

        double mag = std::sqrt (totalMagSq) * (double) gainComp;
        mag = juce::jmax (mag, 1.0e-12);
        filterResponse[band][i] = (float)(20.0 * std::log10 (mag));
    }
}

void SpectrumAnalyzerComponent::update (NewProjectAudioProcessor& proc, int sb, int mask)
{
    selBand = sb;
    if (mask >= 0) selMask = mask; else selMask = (1 << sb);
    sampleRate = proc.getSampleRate();
    for (int i = 0; i < 256; ++i)
        scopeDb[i] = proc.getScopeDb (i);

    // phosphor history — snapshot every ~0.6 s
    if (++ghostTick >= 12)
    {
        ghostTick = 0;
        for (int i = 0; i < kGhostPts; ++i)
            ghost[ghostPos][i] = scopeDb[i * 2];
        ghostValid[ghostPos] = true;
        ghostPos = (ghostPos + 1) % kGhosts;
    }

    for (int b = 0; b < 8; ++b)
    {
        bandEnabled[b] = proc.isBandEnabled (b);
        bandFreq[b]  = proc.getBandFreqHz (b);
        bandRes[b]   = proc.getBandRes01 (b);
        bandPoles[b] = (float) proc.getBandPoles (b);
        bandGateDb[b] = proc.getBandGateDb (b);
        bandOscHz[b] = proc.getBandOscHz (b);
        bandEnv[b]   = proc.getBandEnvLevel (b);
        bandGate[b]  = proc.getBandGateOpen (b);
        if (bandEnabled[b])
            computeFilterResponse (b);
    }
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const auto ink  = lnf ? lnf->theme().ink  : juce::Colour (0xffe8e6de);
    const auto well = lnf ? lnf->theme().well : juce::Colour (0xff111211);

    g.setColour (well);
    g.fillRect (area);

    // plot region (leaves room for axis annotations)
    auto plot = area.reduced (0.0f);
    plot.removeFromLeft (26.0f); plot.removeFromRight (8.0f);
    plot.removeFromTop (10.0f);  plot.removeFromBottom (16.0f);
    const float w = plot.getWidth(), h = plot.getHeight();
    const float px = plot.getX(), py = plot.getY();

    const float minF = 20.0f, maxF = 20000.0f;

    // ── Grid + axes ──
    {
        // minor frequency lines
        g.setColour (ink.withAlpha (0.045f));
        for (float f = 30; f <= 100; f += 10)      g.drawLine (px + hzToX(f,w), py, px + hzToX(f,w), py + h, 1.0f);
        for (float f = 200; f <= 1000; f += 100)   g.drawLine (px + hzToX(f,w), py, px + hzToX(f,w), py + h, 1.0f);
        for (float f = 2000; f <= 20000; f += 1000)g.drawLine (px + hzToX(f,w), py, px + hzToX(f,w), py + h, 1.0f);

        // labelled majors
        struct Tick { float hz; const char* s; };
        const Tick ticks[] = { {50,"50"},{100,"100"},{200,"200"},{500,"500"},
                               {1000,"1K"},{2000,"2K"},{5000,"5K"},{10000,"10K"} };
        if (lnf) g.setFont (lnf->mono (6.2f));
        for (auto& t : ticks)
        {
            const float x = px + hzToX (t.hz, w);
            g.setColour (ink.withAlpha (0.09f));
            g.drawLine (x, py, x, py + h, 1.0f);
            g.setColour (ink.withAlpha (0.35f));
            g.drawText (t.s, (int) x - 12, (int) (py + h) + 4, 24, 8, juce::Justification::centred, false);
        }

        // dB lines + labels
        for (float dB = -48.0f; dB <= 6.0f; dB += 12.0f)
        {
            const float y = py + dbToY (dB, h);
            g.setColour (ink.withAlpha (dB == 0.0f ? 0.13f : 0.055f));
            g.drawLine (px, y, px + w, y, 1.0f);
            g.setColour (ink.withAlpha (0.32f));
            g.drawText (juce::String ((int) dB), 2, (int) y - 4, 21, 8, juce::Justification::centredRight, false);
        }
    }

    // ── Spectrum history (phosphor decay) ──
    for (int k = 1; k <= kGhosts; ++k)
    {
        const int idx = (ghostPos - k + 2 * kGhosts) % kGhosts;   // idx: newest→oldest as k grows
        if (! ghostValid[idx]) continue;
        juce::Path p;
        for (int i = 0; i < kGhostPts; ++i)
        {
            float t = (float) i / (float) (kGhostPts - 1);
            float hz = minF * std::pow (maxF / minF, t);
            float x = px + hzToX (hz, w), y = py + dbToY (ghost[idx][i], h);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (ink.withAlpha (0.10f - 0.02f * (float) k));
        g.strokePath (p, juce::PathStrokeType (0.6f));
    }

    // ── Live spectrum ──
    {
        juce::Path spec;
        spec.startNewSubPath (px, py + h);
        for (int i = 0; i < 256; ++i)
        {
            float t = (float) i / 255.0f;
            float hz = minF * std::pow (maxF / minF, t);
            spec.lineTo (px + hzToX (hz, w), py + dbToY (scopeDb[i], h));
        }
        spec.lineTo (px + w, py + h);
        spec.closeSubPath();
        g.setColour (ink.withAlpha (0.05f));
        g.fillPath (spec);

        juce::Path specLine;
        for (int i = 0; i < 256; ++i)
        {
            float t = (float) i / 255.0f;
            float hz = minF * std::pow (maxF / minF, t);
            float x = px + hzToX (hz, w), y = py + dbToY (scopeDb[i], h);
            if (i == 0) specLine.startNewSubPath (x, y); else specLine.lineTo (x, y);
        }
        g.setColour (ink.withAlpha (0.34f));
        g.strokePath (specLine, juce::PathStrokeType (0.7f));
    }

    // ── Filter response curves ──
    const float clipDb = -48.0f;
    for (int b = 0; b < 8; ++b)
    {
        if (!bandEnabled[b]) continue;

        juce::Path fp;
        bool pathStarted = false;
        for (int i = 0; i < kResponsePts; ++i)
        {
            float dB = filterResponse[b][i];
            if (dB < clipDb) { pathStarted = false; continue; }
            float t = (float) i / (float)(kResponsePts - 1);
            float hz = minF * std::pow (maxF / minF, t);
            float x = px + hzToX (hz, w);
            float y = py + dbToY (dB, h);
            if (!pathStarted) { fp.startNewSubPath (x, y); pathStarted = true; }
            else fp.lineTo (x, y);
        }

        const bool isPrimary  = (b == selBand);
        const bool isSelected = (selMask & (1 << b)) != 0;
        g.setColour (kBandColours[b].withAlpha (isPrimary ? 0.95f : (isSelected ? 0.75f : 0.45f)));
        g.strokePath (fp, juce::PathStrokeType (isPrimary ? 1.4f : (isSelected ? 1.0f : 0.7f)));

        // band centre tick + tag along the top edge (kept clear of the plot borders)
        const float bx = juce::jlimit (px + 4.0f, px + w - 4.0f, px + hzToX (bandFreq[b], w));
        g.setColour (kBandColours[b].withAlpha (isPrimary ? 1.0f : 0.55f));
        g.fillRect (bx - 0.5f, py, 1.0f, isPrimary ? 7.0f : 4.0f);
        if (isSelected)
        {
            if (lnf) g.setFont (lnf->mono (6.2f));
            const float tx = juce::jlimit (px + 10.0f, px + w - 10.0f, bx);
            g.drawText ("B" + juce::String (b + 1), (int) tx - 10, (int) py + 2, 20, 8,
                        juce::Justification::centred, false);
        }
    }

    // ── Detection thresholds (dashed, spanning the band's active region) ──
    for (int b = 0; b < 8; ++b)
    {
        if (!bandEnabled[b]) continue;

        const float threshDb = bandGateDb[b];
        if (threshDb < clipDb || threshDb > 6.0f) continue;

        float fLeft = 0, fRight = 0;
        for (int i = 0; i < kResponsePts; ++i)
            if (filterResponse[b][i] >= threshDb)
            { float t = (float) i / (float)(kResponsePts - 1); fLeft = minF * std::pow (maxF / minF, t); break; }
        for (int i = kResponsePts - 1; i >= 0; --i)
            if (filterResponse[b][i] >= threshDb)
            { float t = (float) i / (float)(kResponsePts - 1); fRight = minF * std::pow (maxF / minF, t); break; }

        if (fLeft > 0 && fRight > fLeft)
        {
            float xL = px + hzToX (fLeft, w);
            float xR = px + hzToX (fRight, w);
            float yTh = py + dbToY (threshDb, h);

            const bool isSelThresh = (selMask & (1 << b)) != 0;
            g.setColour (kBandColours[b].withAlpha (b == selBand ? 0.6f : (isSelThresh ? 0.42f : 0.25f)));
            const float dashLen[] = { 3.0f, 3.0f };
            juce::Path srcPath;
            srcPath.startNewSubPath (xL, yTh);
            srcPath.lineTo (xR, yTh);
            juce::Path dashedPath;
            juce::PathStrokeType (0.6f).createDashedStroke (dashedPath, srcPath, dashLen, 2);
            g.fillPath (dashedPath);
        }
    }

    // ── Live captures — voices sounding right now ──
    for (int b = 0; b < 8; ++b)
    {
        if (!bandEnabled[b] || !bandGate[b] || bandEnv[b] <= 0.01f) continue;
        const float x = px + hzToX (juce::jlimit (minF, maxF, bandOscHz[b]), w);
        const float lift = juce::jlimit (0.0f, 1.0f, bandEnv[b]) * h * 0.85f;
        g.setColour (kBandColours[b].withAlpha (0.55f));
        g.drawLine (x, py + h, x, py + h - lift, 1.0f);
        g.fillEllipse (x - 2.0f, py + h - lift - 2.0f, 4.0f, 4.0f);
    }
}

//==============================================================================
//  (2) EVENT FIELD — time × pitch ethogram
//==============================================================================
float ImpulseSpaceComponent::hzToNorm (float hz)
{
    return juce::jlimit (0.0f, 1.0f,
        std::log2 (juce::jmax (40.0f, hz) / 40.0f) / std::log2 (4000.0f / 40.0f));
}

void ImpulseSpaceComponent::update (NewProjectAudioProcessor& proc, int sb, int mask)
{
    selBand = sb;
    if (mask >= 0) selMaskLocal = mask; else selMaskLocal = (1 << sb);

    head = (head + 1) % kHist;
    for (int b = 0; b < 8; ++b)
    {
        bandEnabled[b] = proc.isBandEnabled (b);
        Frame f;
        f.hz    = proc.getBandOscHz (b);
        f.conf  = proc.getBandConfViz (b);
        f.level = proc.getBandEnvLevel (b);
        f.gate  = proc.getBandGateOpen (b) && bandEnabled[b];
        f.onset = f.gate && ! prevGate[b];
        prevGate[b] = f.gate;
        hist[b][head] = f;

        if (f.onset)
        {
            lastBand = b;
            lastHz   = f.hz;
            lastConf = f.conf;
        }
    }
}

void ImpulseSpaceComponent::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const auto ink  = lnf ? lnf->theme().ink  : juce::Colour (0xffe8e6de);
    const auto well = lnf ? lnf->theme().well : juce::Colour (0xff111211);

    g.setColour (well);
    g.fillRect (area);

    auto plot = area;
    plot.removeFromLeft (26.0f); plot.removeFromRight (8.0f);
    plot.removeFromTop (10.0f);  plot.removeFromBottom (20.0f);
    const float w = plot.getWidth(), h = plot.getHeight();
    const float px = plot.getX(), py = plot.getY();

    // ── Pitch grid: octaves of A ──
    {
        struct Tick { float hz; const char* s; };
        const Tick ticks[] = { {55,"55"},{110,"110"},{220,"220"},{440,"440"},
                               {880,"880"},{1760,"1.7K"},{3520,"3.5K"} };
        if (lnf) g.setFont (lnf->mono (6.0f));
        for (auto& t : ticks)
        {
            const float y = py + h * (1.0f - hzToNorm (t.hz));
            g.setColour (ink.withAlpha (0.07f));
            g.drawLine (px, y, px + w, y, 1.0f);
            g.setColour (ink.withAlpha (0.32f));
            g.drawText (t.s, 1, (int) y - 4, 22, 8, juce::Justification::centredRight, false);
        }
    }

    // ── Time ticks: 1 s intervals; NOW cursor at right ──
    {
        g.setColour (ink.withAlpha (0.22f));
        for (int s = 0; s < kHist; s += 20)
        {
            const float x = px + w * (float) s / (float) (kHist - 1);
            g.drawLine (x, py + h, x, py + h + 3.0f, 1.0f);
        }
        g.setColour (ink.withAlpha (0.18f));
        g.drawLine (px + w, py, px + w, py + h, 1.0f);
        if (lnf) g.setFont (lnf->mono (5.8f));
        g.setColour (ink.withAlpha (0.32f));
        g.drawText ("-6S", (int) px, (int) (py + h) + 4, 20, 7, juce::Justification::centredLeft, false);
        g.drawText ("NOW", (int) (px + w) - 22, (int) (py + h) + 4, 22, 7, juce::Justification::centredRight, false);
    }

    // ── Events per voice ──
    for (int b = 0; b < 8; ++b)
    {
        if (! bandEnabled[b]) continue;
        const bool inSel = (selMaskLocal & (1 << b)) != 0;
        const float emph = inSel ? (b == selBand ? 1.0f : 0.8f) : 0.35f;
        const auto col = kBandColours[b];

        // sustain traces — older half fainter
        juce::Path oldP, newP;
        bool oldPen = false, newPen = false;
        for (int s = 0; s < kHist; ++s)
        {
            const auto& f = hist[b][(head + 1 + s) % kHist];
            const bool newer = s >= kHist / 2;
            juce::Path& p = newer ? newP : oldP;
            bool& pen = newer ? newPen : oldPen;

            if (f.gate && f.hz > 0.0f)
            {
                const float x = px + w * (float) s / (float) (kHist - 1);
                const float y = py + h * (1.0f - hzToNorm (f.hz));
                if (! pen) { p.startNewSubPath (x, y); pen = true; }
                else         p.lineTo (x, y);
            }
            else pen = false;
        }
        g.setColour (col.withAlpha (0.20f * emph));
        g.strokePath (oldP, juce::PathStrokeType (1.0f));
        g.setColour (col.withAlpha (0.45f * emph));
        g.strokePath (newP, juce::PathStrokeType (1.0f));

        // onset events — circle: radius = level, ring opacity = confidence
        for (int s = 0; s < kHist; ++s)
        {
            const auto& f = hist[b][(head + 1 + s) % kHist];
            if (! f.onset || f.hz <= 0.0f) continue;

            const float fade = 0.2f + 0.8f * (float) s / (float) (kHist - 1);
            const float x = px + w * (float) s / (float) (kHist - 1);
            const float y = py + h * (1.0f - hzToNorm (f.hz));
            const float r = 1.5f + juce::jlimit (0.0f, 1.0f, f.level) * 5.5f;

            g.setColour (col.withAlpha ((0.18f + 0.5f * f.conf) * fade * emph));
            g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
            g.setColour (col.withAlpha (0.85f * fade * emph));
            g.drawEllipse (x - r, y - r, r * 2.0f, r * 2.0f, 0.8f);

            // fresh event → crosshair flash
            if (s > kHist - 5)
            {
                g.setColour (ink.withAlpha (0.7f));
                g.drawLine (x - r - 3.0f, y, x - r - 1.0f, y, 1.0f);
                g.drawLine (x + r + 1.0f, y, x + r + 3.0f, y, 1.0f);
            }
        }
    }

    // ── Last event readout ──
    {
        if (lnf) g.setFont (lnf->mono (6.2f));
        juce::String txt = "AWAITING SIGNAL";
        if (lastBand >= 0)
        {
            txt = "LAST B" + juce::String (lastBand + 1)
                + " " + ((lastHz >= 1000.0f) ? juce::String (lastHz / 1000.0f, 2) + "KHZ"
                                             : juce::String ((int) std::round (lastHz)) + "HZ")
                + " C." + juce::String ((int) std::round (juce::jlimit (0.0f, 1.0f, lastConf) * 99.0f)).paddedLeft ('0', 2);
            g.setColour (kBandColours[lastBand].withAlpha (0.9f));
            g.fillRect (px + 26.0f, area.getBottom() - 8.5f, 3.0f, 5.0f);
        }
        g.setColour (ink.withAlpha (0.45f));
        g.drawText (txt, (int) px + 32, (int) area.getBottom() - 11, (int) w - 34, 8,
                    juce::Justification::centredLeft, false);
    }
}

//==============================================================================
//  (3) VOICE VIEW
//==============================================================================
float WaveformDisplayComponent::osc (int waveType, float phase01)
{
    const float t = phase01 - std::floor (phase01);
    switch (waveType)
    {
        case 0: return std::sin (juce::MathConstants<float>::twoPi * t);
        case 1: return 4.0f * std::abs (t - 0.5f) - 1.0f;
        case 2: return 2.0f * t - 1.0f;
        case 3: return (t < 0.5f) ? 1.0f : -1.0f;
    }
    return 0.0f;
}

void WaveformDisplayComponent::update (NewProjectAudioProcessor& proc, int sb, int mask)
{
    selBand = sb;
    if (mask >= 0) selMaskLocal = mask; else selMaskLocal = (1 << sb);

    for (int b = 0; b < 8; ++b)
    {
        if (!(selMaskLocal & (1 << b))) continue;
        auto& bs = bandStates[b];
        bs.bandCol = kBandColours[juce::jlimit(0,7,b)];
        auto* vL = proc.apvts.getRawParameterValue(bandWaveLId(b));
        auto* vR = proc.apvts.getRawParameterValue(bandWaveRId(b));
        auto* vM = proc.apvts.getRawParameterValue(bandWaveMixId(b));
        auto* vN = proc.apvts.getRawParameterValue(bandNoiseId(b));
        auto* vT = proc.apvts.getRawParameterValue(bandToneId(b));
        auto* vTm = proc.apvts.getRawParameterValue(bandTimbreId(b));
        if (vL) bs.waveL = (int) std::round(vL->load());
        if (vR) bs.waveR = (int) std::round(vR->load());
        if (vM) bs.mix = vM->load();
        bs.noiseAmt = vN ? vN->load() : 0.0f;
        bs.toneAmt  = vT ? vT->load() : 0.0f;
        bs.timbre   = vTm ? vTm->load() : 0.5f;
        bs.engine   = proc.getBandEngine (b);
        bs.env      = proc.getBandEnvLevel (b);
    }

    // primary band envelope
    auto rd = [&] (const juce::String& id, float def) {
        auto* p = proc.apvts.getRawParameterValue (id); return p ? p->load() : def; };
    adsr[0] = rd (bandAtkId (sb), 0.01f);
    adsr[1] = rd (bandDecId (sb), 0.3f);
    adsr[2] = rd (bandSusId (sb), 0.7f);
    adsr[3] = rd (bandRelId (sb), 0.5f);
}

void WaveformDisplayComponent::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const auto ink  = lnf ? lnf->theme().ink  : juce::Colour (0xffe8e6de);
    const auto well = lnf ? lnf->theme().well : juce::Colour (0xff111211);

    g.setColour (well);
    g.fillRect (area);

    const auto priCol = kBandColours[juce::jlimit (0, 7, selBand)];

    // ── Tag row ──
    {
        g.setColour (priCol);
        g.fillRect (area.getX() + 8.0f, area.getY() + 8.0f, 3.0f, 8.0f);
        if (lnf) g.setFont (lnf->monoBold (8.0f, 0.06f));
        g.setColour (ink.withAlpha (0.85f));
        g.drawText ("B" + juce::String (selBand + 1) + " - " + juce::String (voiceEngineName (bandStates[selBand].engine)),
                    (int) area.getX() + 15, (int) area.getY() + 7, 140, 10,
                    juce::Justification::centredLeft, false);
        if (lnf) g.setFont (lnf->mono (6.2f));
        g.setColour (ink.withAlpha (0.35f));
        g.drawText ("1 CYCLE", (int) area.getRight() - 60, (int) area.getY() + 8, 52, 8,
                    juce::Justification::centredRight, false);
    }

    // ── Wave plot ──
    auto wave = area;
    wave.removeFromTop (24.0f);
    auto envStrip = wave.removeFromBottom (48.0f);
    wave.reduce (8.0f, 2.0f);

    const float w = wave.getWidth(), h = wave.getHeight();
    const float mid = wave.getCentreY();

    g.setColour (ink.withAlpha (0.12f));
    g.drawLine (wave.getX(), mid, wave.getRight(), mid, 1.0f);
    // quarter-phase ticks
    g.setColour (ink.withAlpha (0.18f));
    for (int q = 0; q <= 4; ++q)
    {
        const float x = wave.getX() + w * (float) q / 4.0f;
        g.drawLine (x, wave.getBottom(), x, wave.getBottom() - 3.0f, 1.0f);
    }

    for (int b = 0; b < 8; ++b)
    {
        if (!(selMaskLocal & (1 << b))) continue;
        const auto& bs = bandStates[b];
        const bool isPrimary = (b == selBand);
        const float excite = juce::jlimit (0.0f, 1.0f, bs.env);
        const float alphaScale = (isPrimary ? 1.0f : 0.5f) * (0.55f + 0.45f * excite);

        auto shape = [&] (float phase) -> float
        {
            float val = TrackSynth::previewEngine (bs.engine, phase, bs.mix, bs.timbre,
                                                   bs.waveL, bs.waveR, bs.mix);
            if (bs.noiseAmt > 0.001f) val *= (1.0f - bs.noiseAmt);
            if (bs.toneAmt > 0.001f)  val = TrackSynth::applySaturation (val, bs.toneAmt);
            return val;
        };

        // stochastic band (noise share drawn as a corridor around the cycle)
        if (bs.noiseAmt > 0.01f)
        {
            const int numPts = (int) w;
            const float noiseBand = bs.noiseAmt * 0.42f * h;
            juce::Path band;
            for (int i = 0; i <= numPts; ++i)
            {
                float yC = mid - shape ((float) i / (float) numPts) * (h * 0.42f);
                if (i == 0) band.startNewSubPath (wave.getX() + (float) i, yC - noiseBand * 0.5f);
                else        band.lineTo (wave.getX() + (float) i, yC - noiseBand * 0.5f);
            }
            for (int i = numPts; i >= 0; --i)
            {
                float yC = mid - shape ((float) i / (float) numPts) * (h * 0.42f);
                band.lineTo (wave.getX() + (float) i, yC + noiseBand * 0.5f);
            }
            band.closeSubPath();
            g.setColour (bs.bandCol.withAlpha (0.13f * alphaScale));
            g.fillPath (band);
        }

        juce::Path path;
        const int numPts = (int) w;
        for (int i = 0; i <= numPts; ++i)
        {
            float y = mid - shape ((float) i / (float) numPts) * (h * 0.42f);
            if (i == 0) path.startNewSubPath (wave.getX() + (float) i, y);
            else        path.lineTo (wave.getX() + (float) i, y);
        }
        g.setColour (bs.bandCol.withAlpha (0.9f * alphaScale));
        g.strokePath (path, juce::PathStrokeType (isPrimary ? 1.3f : 0.8f));
    }

    // ── Envelope profile (primary voice) ──
    {
        auto strip = envStrip.reduced (8.0f, 0.0f);
        auto lane  = strip.withTrimmedBottom (12.0f).withTrimmedTop (6.0f);

        g.setColour (ink.withAlpha (0.08f));
        g.drawLine (strip.getX(), strip.getY(), strip.getRight(), strip.getY(), 1.0f);

        const float A = juce::jmax (adsr[0], 0.02f), D = juce::jmax (adsr[1], 0.02f);
        const float S = juce::jlimit (0.0f, 1.0f, adsr[2]), R = juce::jmax (adsr[3], 0.02f);
        const float total = A + D + R;
        const float wHold = lane.getWidth() * 0.22f;
        const float wRest = lane.getWidth() - wHold;
        const float xA = lane.getX() + wRest * (A / total);
        const float xD = xA + wRest * (D / total);
        const float xS = xD + wHold;
        const float yS = lane.getBottom() - S * lane.getHeight();

        juce::Path env;
        env.startNewSubPath (lane.getX(), lane.getBottom());
        env.lineTo (xA, lane.getY());
        env.lineTo (xD, yS);
        env.lineTo (xS, yS);
        env.lineTo (lane.getRight(), lane.getBottom());

        juce::Path fill (env);
        fill.lineTo (lane.getX(), lane.getBottom());
        fill.closeSubPath();
        g.setColour (priCol.withAlpha (0.10f));
        g.fillPath (fill);
        g.setColour (ink.withAlpha (0.5f));
        g.strokePath (env, juce::PathStrokeType (1.0f));

        auto fmtT = [] (float s) -> juce::String {
            if (s < 0.01f) return "0MS";
            if (s < 1.0f)  return juce::String ((int) std::round (s * 1000.0f)) + "MS";
            return juce::String (s, 2) + "S"; };

        if (lnf) g.setFont (lnf->mono (6.0f));
        g.setColour (ink.withAlpha (0.42f));
        g.drawText ("A " + fmtT (adsr[0]) + "  D " + fmtT (adsr[1])
                    + "  S " + juce::String ((int) std::round (S * 100.0f)) + "%"
                    + "  R " + fmtT (adsr[3]),
                    (int) strip.getX(), (int) strip.getBottom() - 9, (int) strip.getWidth() - 26, 8,
                    juce::Justification::centredLeft, false);
        if (lnf) g.setFont (lnf->grot (6.5f, 0.10f));
        g.setColour (ink.withAlpha (0.35f));
        g.drawText ("ENV", (int) strip.getRight() - 24, (int) strip.getBottom() - 9, 24, 8,
                    juce::Justification::centredRight, false);
    }
}

//==============================================================================
//  Constructor
//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);
    setSize (1019, 650);
    createToucanDrawable();

    for (int i=0;i<8;++i){ bandSelBtns[i].bandIndex=i; bandSelBtns[i].lnf=&lnf; bandSelBtns[i].setClickingTogglesState(false);
        bandSelBtns[i].setWantsKeyboardFocus(false);
        bandSelBtns[i].onClick=[this,i]{
            handleBandClick(i, juce::ModifierKeys::currentModifiers);
        }; addAndMakeVisible(bandSelBtns[i]); }
    bandSelBtns[0].setToggleState(true,juce::dontSendNotification);
    for(int i=0;i<8;++i){ bandLampBtns[i].bandIndex=i; bandLampBtns[i].setWantsKeyboardFocus(false); addAndMakeVisible(bandLampBtns[i]);
        bandOnAtts[i]=std::make_unique<ButtonAttachment>(audioProcessor.apvts,bandOnId(i),bandLampBtns[i]); }

    scaleModeBtn.lnf=&lnf; scaleModeBtn.setClickingTogglesState(true); scaleModeBtn.setWantsKeyboardFocus(false); addAndMakeVisible(scaleModeBtn);
    scaleModeAtt=std::make_unique<ButtonAttachment>(audioProcessor.apvts,kScaleMinorId,scaleModeBtn);
    waveBtnL.setWantsKeyboardFocus(false); waveBtnR.setWantsKeyboardFocus(false);
    waveBtnL.onClick=[this]{cycleWave(true);}; waveBtnR.onClick=[this]{cycleWave(false);};
    addChildComponent(waveBtnL); addChildComponent(waveBtnR);  // lean model: hidden (presets drive Blend waves)

    // Engine selector (per selected band)
    engineSel.lnfPtr = &lnf;
    engineSel.onPick = [this](int e){ setEngineForSelectedBands(e); };
    addAndMakeVisible(engineSel);
    for(int i=0;i<12;++i){ noteBtns[i].pitchClass=i; noteBtns[i].lnf=&lnf; noteBtns[i].setClickingTogglesState(true); noteBtns[i].setWantsKeyboardFocus(false);
        noteBtns[i].onClick=[this]{writeNoteButtonsToParam();}; addAndMakeVisible(noteBtns[i]); }
    octaveCtrl.lnf=&lnf; addAndMakeVisible(octaveCtrl);
    mixer.lnf=&lnf; addAndMakeVisible(mixer); mixer.attachVolumes(audioProcessor.apvts);

    // Header buttons — styled by the LookAndFeel
    presetBtn.setButtonText("PRESETS");
    presetBtn.setClickingTogglesState(false);
    presetBtn.setWantsKeyboardFocus(false);
    presetBtn.onClick=[this]{ showPresetMenu(); };
    addAndMakeVisible(presetBtn);

    midiOutBtn.setButtonText("MIDI OUT");
    midiOutBtn.setClickingTogglesState(true);
    midiOutBtn.setWantsKeyboardFocus(false);
    addAndMakeVisible(midiOutBtn);
    midiOutAtt = std::make_unique<ButtonAttachment>(audioProcessor.apvts, kMidiOutId, midiOutBtn);

    // Visualizations
    spectrumViz.lnf = &lnf;
    spaceViz.lnf    = &lnf;
    waveformViz.lnf = &lnf;
    addAndMakeVisible (spectrumViz);
    addAndMakeVisible (spaceViz);
    addAndMakeVisible (waveformViz);

    for(auto*s:{&knobScaleRoot,&knobWidth,&knobDryWet}){setupKnob(*s);addAndMakeVisible(*s);}
    attScaleRoot=std::make_unique<SliderAttachment>(audioProcessor.apvts,kScaleRootId,knobScaleRoot);
    attWidth=std::make_unique<SliderAttachment>(audioProcessor.apvts,kWidthId,knobWidth);
    attDryWet=std::make_unique<SliderAttachment>(audioProcessor.apvts,kDryWetId,knobDryWet);
    for(auto*s:{&knobRandom,&knobReverb}){setupKnob(*s);addAndMakeVisible(*s);}
    attReverb=std::make_unique<SliderAttachment>(audioProcessor.apvts,kReverbId,knobReverb);
    attRandom=std::make_unique<SliderAttachment>(audioProcessor.apvts,kRandomId,knobRandom);
    for(auto*s:{&knobDrive,&knobDelayMix,&knobDelayTime,&knobDelayFb,&knobComp}){setupKnob(*s);addAndMakeVisible(*s);}
    attDrive    =std::make_unique<SliderAttachment>(audioProcessor.apvts,kDriveId,knobDrive);
    attDelayMix =std::make_unique<SliderAttachment>(audioProcessor.apvts,kDelayMixId,knobDelayMix);
    attDelayTime=std::make_unique<SliderAttachment>(audioProcessor.apvts,kDelayTimeId,knobDelayTime);
    attDelayFb  =std::make_unique<SliderAttachment>(audioProcessor.apvts,kDelayFbId,knobDelayFb);
    attComp     =std::make_unique<SliderAttachment>(audioProcessor.apvts,kCompId,knobComp);
    for(auto*s:{&knobHP,&knobTP,&knobTone,&knobNoise}){setupKnob(*s);addAndMakeVisible(*s);}
    for(auto*s:{&knobSnap,&knobFreq,&knobRes,&knobSlope,&knobThreshold,&knobTolerance,&knobSensitivity,&knobConfidence,
                &knobWaveform,&knobTimbre,&knobA,&knobD,&knobS,&knobR}){setupKnob(*s);addAndMakeVisible(*s);}

    lastScaleRoot=(int)std::round(readParam(kScaleRootId));
    lastScaleMinor=(readParam(kScaleMinorId)>=.5f);
    setSelectedBand(0);
    installKnobPropagation();
    startTimerHz(20);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() {
    stopTimer();
    attSnap.reset();attFreq.reset();attRes.reset();attSlope.reset();
    attThreshold.reset();attTolerance.reset();attSensitivity.reset();attConfidence.reset();
    attWaveform.reset();attTimbre.reset();attA.reset();attD.reset();attS.reset();attR.reset();
    attNoise.reset();attTone.reset();attHP.reset();attLP.reset();
    attScaleRoot.reset();attWidth.reset();attDryWet.reset();
    attReverb.reset();attRandom.reset();
    attDrive.reset();attDelayMix.reset();attDelayTime.reset();attDelayFb.reset();attComp.reset();
    midiOutAtt.reset();
    octaveCtrl.detachParam();
    for(int i=0;i<8;++i) mixer.volAtts[i].reset();
    for(int i=0;i<8;++i) bandOnAtts[i].reset();
    scaleModeAtt.reset();
    setLookAndFeel(nullptr);
}

//==============================================================================
void NewProjectAudioProcessorEditor::setSelectedBand(int band){
    selectedBand=juce::jlimit(0,7,band);
    bandSelMask = (1 << selectedBand);   // plain click = single selection
    lastClickedBand = selectedBand;
    for(int i=0;i<8;++i){
        bandSelBtns[i].setToggleState(i==selectedBand,juce::dontSendNotification);
        bandSelBtns[i].multiSelected=false;
    }
    rebindPrimaryBand(selectedBand);
}

//==============================================================================
void NewProjectAudioProcessorEditor::handleBandClick(int band, const juce::ModifierKeys& mods){
    band = juce::jlimit(0,7,band);

    if (mods.isCommandDown()) {
        // CMD/Ctrl+click: toggle individual band in/out of selection
        int bit = (1 << band);
        if (bandSelMask & bit) {
            int newMask = bandSelMask & ~bit;
            if (newMask == 0) return; // can't deselect the last one
            bandSelMask = newMask;
            if (band == selectedBand) {
                for (int i=0;i<8;++i) {
                    if (bandSelMask & (1<<i)) { selectedBand=i; break; }
                }
                rebindPrimaryBand(selectedBand);
            }
        } else {
            bandSelMask |= bit;
            selectedBand = band;
            rebindPrimaryBand(selectedBand);
        }
        lastClickedBand = band;
    }
    else if (mods.isShiftDown()) {
        // SHIFT+click: range selection from lastClickedBand to this band
        int lo = juce::jmin(lastClickedBand, band);
        int hi = juce::jmax(lastClickedBand, band);
        bandSelMask = 0;
        for (int i=lo; i<=hi; ++i) bandSelMask |= (1<<i);
        selectedBand = band;
        rebindPrimaryBand(selectedBand);
        // Don't update lastClickedBand — SHIFT extends from the anchor
    }
    else {
        // Plain click: single selection
        selectedBand = band;
        bandSelMask = (1 << band);
        lastClickedBand = band;
        rebindPrimaryBand(selectedBand);
    }

    // Update button visuals
    for(int i=0;i<8;++i){
        bool isPrimary = (i == selectedBand);
        bool isInMask  = (bandSelMask & (1<<i)) != 0;
        bandSelBtns[i].setToggleState(isPrimary, juce::dontSendNotification);
        bandSelBtns[i].multiSelected = (!isPrimary && isInMask);
    }
    updateWaveButtons(); syncNoteButtonsFromParam(); repaint();
}

//==============================================================================
void NewProjectAudioProcessorEditor::applyVoiceTint (int band)
{
    const auto col = kBandColours[juce::jlimit (0, 7, band)];
    for (auto* s : { &knobSnap, &knobFreq, &knobRes, &knobSlope,
                     &knobThreshold, &knobTolerance, &knobSensitivity, &knobConfidence,
                     &knobWaveform, &knobTimbre, &knobA, &knobD, &knobS, &knobR,
                     &knobHP, &knobTP, &knobTone, &knobNoise })
        s->setColour (juce::Slider::rotarySliderFillColourId, col);

    octaveCtrl.accent = col;
    engineSel.accent  = col;
}

//==============================================================================
void NewProjectAudioProcessorEditor::rebindPrimaryBand(int band){
    attSnap.reset();attFreq.reset();attRes.reset();attSlope.reset();
    attThreshold.reset();attTolerance.reset();attSensitivity.reset();attConfidence.reset();
    attWaveform.reset();attTimbre.reset();attA.reset();attD.reset();attS.reset();attR.reset();
    attNoise.reset();attTone.reset();attHP.reset();attLP.reset();
    octaveCtrl.detachParam();
    const int b=band;
    attSnap=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandSnapId(b),knobSnap);
    attFreq=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandFreqId(b),knobFreq);
    attRes=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandResId(b),knobRes);
    attSlope=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandPoleId(b),knobSlope);
    attThreshold=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandGateId(b),knobThreshold);
    attTolerance=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandTolId(b),knobTolerance);
    attSensitivity=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandSensId(b),knobSensitivity);
    attConfidence=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandConfId(b),knobConfidence);
    attWaveform=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandWaveMixId(b),knobWaveform);
    attTimbre=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandTimbreId(b),knobTimbre);
    attA=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandAtkId(b),knobA);
    attD=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandDecId(b),knobD);
    attS=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandSusId(b),knobS);
    attR=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandRelId(b),knobR);
    attNoise=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandNoiseId(b),knobNoise);
    attTone=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandToneId(b),knobTone);
    attHP=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandHPId(b),knobHP);
    attLP=std::make_unique<SliderAttachment>(audioProcessor.apvts,bandLPId(b),knobTP);
    octaveCtrl.attachToParam(audioProcessor.apvts,bandOctId(b));
    engineSel.current = audioProcessor.getBandEngine(b);
    applyVoiceTint(b);
    engineSel.repaint();
    updateWaveButtons();syncNoteButtonsFromParam();repaint();
}

//==============================================================================
void NewProjectAudioProcessorEditor::propagateKnobToSelectedBands(
    juce::String (*paramIdFunc)(int), juce::Slider& knob)
{
    if (propagating) return;
    // Only propagate when the user is actively dragging/interacting with the knob.
    // This prevents overwriting other bands' values when attachments rebind on band switch.
    if (!knob.isMouseButtonDown() && !knob.isMouseOver()) return;

    propagating = true;

    const float val = (float) knob.getValue();
    for (int i = 0; i < 8; ++i)
    {
        if (i == selectedBand) continue;
        if (!(bandSelMask & (1 << i))) continue;

        const auto pid = paramIdFunc(i);
        auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(pid));
        if (p)
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(val));
            p->endChangeGesture();
        }
    }
    propagating = false;
}

//==============================================================================
void NewProjectAudioProcessorEditor::installKnobPropagation()
{
    knobSnap.onValueChange       = [this]{ propagateKnobToSelectedBands(bandSnapId, knobSnap); };
    knobFreq.onValueChange       = [this]{ propagateKnobToSelectedBands(bandFreqId, knobFreq); };
    knobRes.onValueChange        = [this]{ propagateKnobToSelectedBands(bandResId, knobRes); };
    knobSlope.onValueChange      = [this]{ propagateKnobToSelectedBands(bandPoleId, knobSlope); };
    knobThreshold.onValueChange  = [this]{ propagateKnobToSelectedBands(bandGateId, knobThreshold); };
    knobTolerance.onValueChange  = [this]{ propagateKnobToSelectedBands(bandTolId, knobTolerance); };
    knobSensitivity.onValueChange= [this]{ propagateKnobToSelectedBands(bandSensId, knobSensitivity); };
    knobConfidence.onValueChange = [this]{ propagateKnobToSelectedBands(bandConfId, knobConfidence); };
    knobWaveform.onValueChange   = [this]{ propagateKnobToSelectedBands(bandWaveMixId, knobWaveform); };
    knobTimbre.onValueChange     = [this]{ propagateKnobToSelectedBands(bandTimbreId, knobTimbre); };
    knobA.onValueChange          = [this]{ propagateKnobToSelectedBands(bandAtkId, knobA); };
    knobD.onValueChange          = [this]{ propagateKnobToSelectedBands(bandDecId, knobD); };
    knobS.onValueChange          = [this]{ propagateKnobToSelectedBands(bandSusId, knobS); };
    knobR.onValueChange          = [this]{ propagateKnobToSelectedBands(bandRelId, knobR); };
    knobNoise.onValueChange      = [this]{ propagateKnobToSelectedBands(bandNoiseId, knobNoise); };
    knobTone.onValueChange       = [this]{ propagateKnobToSelectedBands(bandToneId, knobTone); };
    knobHP.onValueChange         = [this]{ propagateKnobToSelectedBands(bandHPId, knobHP); };
    knobTP.onValueChange         = [this]{ propagateKnobToSelectedBands(bandLPId, knobTP); };
}

void NewProjectAudioProcessorEditor::setEngineForSelectedBands(int engineIdx){
    engineIdx = juce::jlimit(0, kNumEngines-1, engineIdx);
    for(int i=0;i<8;++i){
        if(!(bandSelMask & (1<<i))) continue;
        auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandEngineId(i)));
        if(p){ p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1((float)engineIdx)); p->endChangeGesture(); }
    }
    engineSel.current = engineIdx;
    engineSel.repaint();
    repaint();
}

void NewProjectAudioProcessorEditor::showPresetMenu(){
    juce::PopupMenu m;
    m.setLookAndFeel(&lnf);

    // Factory presets: ids 1..N
    juce::PopupMenu factory;
    for(int i=0;i<NewProjectAudioProcessor::kNumPresets;++i)
        factory.addItem(i+1, NewProjectAudioProcessor::presetName(i));
    m.addSubMenu("Factory", factory);

    // User presets: ids 100+
    auto userNames = audioProcessor.getUserPresetNames();
    if(userNames.size()>0){
        juce::PopupMenu user;
        for(int i=0;i<userNames.size();++i)
            user.addItem(100+i, userNames[i]);
        m.addSubMenu("User", user);
    }

    m.addSeparator();
    m.addItem(1000, "Save current as preset...");
    m.addItem(1001, "Open presets folder");

    auto refresh = [this]{
        engineSel.current = audioProcessor.getBandEngine(selectedBand);
        applyVoiceTint(selectedBand);
        updateWaveButtons(); syncNoteButtonsFromParam();
        engineSel.repaint(); repaint();
    };

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetBtn),
        [this,userNames,refresh](int res){
            if(res<=0) return;
            if(res>=1 && res<=NewProjectAudioProcessor::kNumPresets){
                audioProcessor.applyPreset(res-1); refresh();
            }
            else if(res>=100 && res<100+userNames.size()){
                audioProcessor.loadUserPreset(userNames[res-100]); refresh();
            }
            else if(res==1000){
                auto* w = new juce::AlertWindow("Save Preset",
                            "Name your preset:", juce::MessageBoxIconType::NoIcon);
                w->setLookAndFeel(&lnf);
                w->addTextEditor("name", "My Soundscape");
                w->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
                w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                w->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this,w](int r){
                        if(r==1){
                            auto nm = w->getTextEditorContents("name").trim();
                            if(nm.isNotEmpty()) audioProcessor.saveUserPreset(nm);
                        }
                        w->setLookAndFeel(nullptr);
                        delete w;
                    }), false);
            }
            else if(res==1001){
                NewProjectAudioProcessor::getUserPresetDir().revealToUser();
            }
        });
}

void NewProjectAudioProcessorEditor::cycleWave(bool left){
    const auto&id=left?bandWaveLId(selectedBand):bandWaveRId(selectedBand);
    const auto&otherId=left?bandWaveRId(selectedBand):bandWaveLId(selectedBand);
    auto*p=dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(id));
    if(!p)return;
    int cur=(int)std::round(audioProcessor.apvts.getRawParameterValue(id)->load());
    int other=(int)std::round(audioProcessor.apvts.getRawParameterValue(otherId)->load());
    int next=(cur+1)&3;
    if(next==other) next=(next+1)&3; // skip the other side's waveform
    p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1((float)next));p->endChangeGesture();updateWaveButtons(); }
void NewProjectAudioProcessorEditor::updateWaveButtons(){
    auto*vL=audioProcessor.apvts.getRawParameterValue(bandWaveLId(selectedBand));
    auto*vR=audioProcessor.apvts.getRawParameterValue(bandWaveRId(selectedBand));
    if(vL)waveBtnL.currentWave=(int)std::round(vL->load()); if(vR)waveBtnR.currentWave=(int)std::round(vR->load());
    waveBtnL.repaint();waveBtnR.repaint(); }
void NewProjectAudioProcessorEditor::syncNoteButtonsFromParam(){
    const int mask=(int)std::round(readParam(bandNoteMaskId(selectedBand)));
    for(int i=0;i<12;++i)noteBtns[i].setToggleState(((mask>>i)&1)!=0,juce::dontSendNotification); }
void NewProjectAudioProcessorEditor::writeNoteButtonsToParam(){
    int mask=0; for(int i=0;i<12;++i)if(noteBtns[i].getToggleState())mask|=(1<<i);
    auto*p=dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandNoteMaskId(selectedBand)));
    if(p){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1((float)mask));p->endChangeGesture();} }
void NewProjectAudioProcessorEditor::resetAllBandNotesToScale(){
    const int root=(int)std::round(readParam(kScaleRootId)); const bool minor=(readParam(kScaleMinorId)>=.5f);
    const int mask=ScaleQuantizer::scaleToBitmask(root,minor);
    for(int b=0;b<8;++b){auto*p=dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandNoteMaskId(b)));
        if(p){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1((float)mask));p->endChangeGesture();}}
    syncNoteButtonsFromParam(); }

void NewProjectAudioProcessorEditor::timerCallback(){
    const int curRoot=(int)std::round(readParam(kScaleRootId)); const bool curMinor=(readParam(kScaleMinorId)>=.5f);
    if(curRoot!=lastScaleRoot||curMinor!=lastScaleMinor){lastScaleRoot=curRoot;lastScaleMinor=curMinor;resetAllBandNotesToScale();}
    for(int i=0;i<12;++i)noteBtns[i].glow=audioProcessor.getNoteGlow(i);
    for(int i=0;i<8;++i){mixer.setLevels(i,audioProcessor.getBandLevelL(i),audioProcessor.getBandLevelR(i));mixer.setBandOn(i,audioProcessor.isBandEnabled(i));}

    // Keep engine selector in sync with the primary band (automation / presets)
    { int e = audioProcessor.getBandEngine(selectedBand);
      if (e != engineSel.current) { engineSel.current = e; engineSel.repaint(); } }

    // Update visualizations (pass selection mask for multi-band overlay)
    spectrumViz.update (audioProcessor, selectedBand, bandSelMask);
    spaceViz.update (audioProcessor, selectedBand, bandSelMask);
    waveformViz.update (audioProcessor, selectedBand, bandSelMask);

    // MIDI OUT mode: grey out synth, mixer, reverb, dry/wet
    const bool midiOn = midiOutBtn.getToggleState();
    const float midiAlpha = midiOn ? 0.25f : 1.0f;
    for(auto*s:{&knobWaveform,&knobA,&knobD,&knobS,&knobR,&knobHP,&knobTP,&knobTone,&knobNoise})
    { s->setAlpha(midiAlpha); s->setEnabled(!midiOn); }
    waveBtnL.setAlpha(midiAlpha); waveBtnL.setEnabled(!midiOn);
    waveBtnR.setAlpha(midiAlpha); waveBtnR.setEnabled(!midiOn);
    mixer.setAlpha(midiAlpha); mixer.setEnabled(!midiOn);
    waveformViz.setAlpha(midiAlpha);
    knobReverb.setAlpha(midiAlpha); knobReverb.setEnabled(!midiOn);
    knobDryWet.setAlpha(midiAlpha); knobDryWet.setEnabled(!midiOn);
    for(auto*s:{&knobDrive,&knobDelayMix,&knobDelayTime,&knobDelayFb,&knobComp})
    { s->setAlpha(midiAlpha); s->setEnabled(!midiOn); }

    updateWaveButtons();repaint(); }

//==============================================================================
//  Layout
//==============================================================================
void NewProjectAudioProcessorEditor::resized()
{
    zHeader   = { 0, 0, 1019, 40 };
    zVoices   = { 12, 52, 48, 328 };
    zAnalysis = { 68, 52, 516, 328 };
    zQuant    = { 592, 52, 415, 96 };
    zSynth    = { 592, 156, 415, 224 };
    zFilter   = { 12, 388, 272, 104 };
    zImpulse  = { 12, 500, 272, 124 };
    zEvents   = { 292, 388, 238, 236 };
    zMix      = { 538, 388, 240, 236 };
    zFx       = { 786, 388, 221, 236 };
    zFooter   = { 0, 634, 1019, 16 };

    // Header buttons + brand
    presetBtn.setBounds  (845, 10, 58, 20);
    midiOutBtn.setBounds (909, 10, 62, 20);

    // Voice rail — 8 cells, lamp overlaps each cell's right side
    for (int i = 0; i < 8; ++i)
    {
        const int y = 66 + i * 39;
        bandSelBtns[i].setBounds (12, y, 48, 37);
        bandLampBtns[i].setBounds (43, y + 12, 14, 13);
    }

    // Analysis
    spectrumViz.setBounds (zAnalysis.reduced (1));

    // Quantizer
    knobScaleRoot.setBounds (604, 72, 36, 36);
    scaleModeBtn.setBounds  (654, 82, 60, 16);
    knobSnap.setBounds      (736, 72, 36, 36);
    {
        const int natPC[7] = { 0,2,4,5,7,9,11 };
        const int shpPC[5] = { 1,3,6,8,10 };
        const int kx = 788, nw = 24;
        for (int i = 0; i < 7; ++i)
            noteBtns[natPC[i]].setBounds (kx + i * nw, 92, nw, 21);
        const int shpAfter[5] = { 0, 1, 3, 4, 5 };   // sharps sit on these boundaries
        for (int i = 0; i < 5; ++i)
            noteBtns[shpPC[i]].setBounds (kx + (shpAfter[i] + 1) * nw - 10, 68, 20, 19);
    }
    octaveCtrl.setBounds (966, 70, 40, 40);

    // Synth
    engineSel.setBounds    (604, 170, 172, 20);
    knobWaveform.setBounds (627, 210, 40, 40);   // MORPH
    knobTimbre.setBounds   (713, 210, 40, 40);
    {
        const int xs[4] = { 613, 656, 699, 742 };
        juce::Slider* adsr[4] = { &knobA, &knobD, &knobS, &knobR };
        for (int i = 0; i < 4; ++i) adsr[i]->setBounds (xs[i], 288, 24, 24);
        const int xs2[4] = { 611, 654, 697, 740 };
        juce::Slider* row[4] = { &knobHP, &knobTP, &knobTone, &knobNoise };
        for (int i = 0; i < 4; ++i) row[i]->setBounds (xs2[i], 328, 28, 28);
    }
    waveformViz.setBounds (784, 170, 215, 202);

    // Hidden legacy wave selectors (function preserved; presets drive Blend waves)
    waveBtnL.setBounds (628, 224, 29, 24);
    waveBtnR.setBounds (721, 224, 30, 26);

    // Filter
    knobFreq.setBounds  (26, 404, 44, 44);
    knobRes.setBounds   (114, 408, 36, 36);
    knobSlope.setBounds (198, 408, 36, 36);

    // Impulse detection
    knobThreshold.setBounds   (30, 520, 32, 32);
    knobSensitivity.setBounds (98, 520, 32, 32);
    knobTolerance.setBounds   (166, 520, 32, 32);
    knobConfidence.setBounds  (234, 520, 32, 32);

    // Events
    spaceViz.setBounds (zEvents.reduced (1));

    // Mix
    mixer.setBounds (548, 402, 220, 212);

    // Effects — 3×3 grid
    {
        const int gx[3] = { 804, 878, 951 }, gy[3] = { 404, 478, 552 };
        const int kw = 38, kh = 38;
        knobWidth.setBounds   (gx[0], gy[0], kw, kh); knobDryWet.setBounds  (gx[1], gy[0], kw, kh); knobRandom.setBounds (gx[2], gy[0], kw, kh);
        knobDrive.setBounds   (gx[0], gy[1], kw, kh); knobDelayMix.setBounds(gx[1], gy[1], kw, kh); knobDelayTime.setBounds(gx[2], gy[1], kw, kh);
        knobDelayFb.setBounds (gx[0], gy[2], kw, kh); knobReverb.setBounds  (gx[1], gy[2], kw, kh); knobComp.setBounds   (gx[2], gy[2], kw, kh);
    }
}

//==============================================================================
//  Painting
//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (lnf.theme().carbon);
    drawHeader (g);
    drawCaptions (g);
}

void NewProjectAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    drawFrames (g);
}

void NewProjectAudioProcessorEditor::drawHeader (juce::Graphics& g) const
{
    const auto ink = lnf.theme().ink;

    // Wordmark
    auto fGems = lnf.grot (21.0f, 0.02f);
    g.setFont (fGems);
    g.setColour (ink.withAlpha (0.96f));
    g.drawText ("GEMS", 12, 9, 70, 22, juce::Justification::centredLeft, false);
    const float gemsW = juce::GlyphArrangement::getStringWidth (fGems, "GEMS");

    g.setFont (lnf.monoBold (7.0f));
    g.setColour (ink.withAlpha (0.5f));
    g.drawText ("V2", (int) gemsW + 16, 10, 16, 8, juce::Justification::centredLeft, false);

    const float sepX = gemsW + 38.0f;
    g.setColour (lnf.theme().hairline);
    g.drawLine (sepX, 10.0f, sepX, 30.0f, 1.0f);

    g.setFont (lnf.mono (6.4f, 0.06f));
    g.setColour (ink.withAlpha (0.5f));
    g.drawText ("GENERATIVE ECOACOUSTIC", (int) sepX + 10, 12, 160, 8, juce::Justification::centredLeft, false);
    g.drawText ("MODULAR SOUNDSCAPE",     (int) sepX + 10, 21, 160, 8, juce::Justification::centredLeft, false);

    // Signal chain — the process, spelled out
    {
        struct Tok { const char* s; int kind; };   // 0 dim name, 1 index, 2 arrow
        const Tok toks[] = {
            {"IN",1},{">",2},{"01",1},{"FILTER",0},{">",2},{"02",1},{"IMPULSE",0},{">",2},
            {"03",1},{"QUANTIZE",0},{">",2},{"04",1},{"SYNTH",0},{">",2},{"05",1},{"FX",0},{">",2},
            {"06",1},{"MIX",0},{">",2},{"OUT",1} };

        auto fN = lnf.mono (7.0f);
        auto fB = lnf.monoBold (7.0f);

        float total = 0.0f;
        for (auto& t : toks)
            total += juce::GlyphArrangement::getStringWidth (t.kind == 1 ? fB : fN, t.s) + 5.0f;

        float x = 833.0f - total;
        for (auto& t : toks)
        {
            const auto& f = (t.kind == 1 ? fB : fN);
            const float tw = juce::GlyphArrangement::getStringWidth (f, t.s);
            g.setFont (f);
            g.setColour (ink.withAlpha (t.kind == 1 ? 0.85f : t.kind == 0 ? 0.42f : 0.22f));
            g.drawText (t.s, (int) x, 15, (int) tw + 2, 10, juce::Justification::centredLeft, false);
            x += tw + 5.0f;
        }
    }

    // Studio mark
    if (toucanDrawable)
        toucanDrawable->drawWithin (g, juce::Rectangle<float> (979.0f, 7.0f, 26.0f, 26.0f),
                                    juce::RectanglePlacement::centred, 0.9f);

    g.setColour (lnf.theme().hairline);
    g.fillRect (0, 40, getWidth(), 1);

    // Footer
    g.fillRect (0, 634, getWidth(), 1);
    g.setFont (lnf.mono (6.4f, 0.06f));
    g.setColour (ink.withAlpha (0.32f));
    g.drawText ("BIOACOUSTIC RE-SYNTHESIS INSTRUMENT", 12, 639, 300, 8, juce::Justification::centredLeft, false);
    g.setColour (ink.withAlpha (0.5f));
    g.drawText ("TOTEMPHONIA STUDIO BERLIN", getWidth() - 212, 639, 200, 8, juce::Justification::centredRight, false);
}

void NewProjectAudioProcessorEditor::drawFrames (juce::Graphics& g) const
{
    lnf.drawModuleFrame (g, zAnalysis, "01-02", "ANALYSIS",  "FFT 2048 / HANN / 20HZ-20KHZ");
    lnf.drawModuleFrame (g, zQuant,    "03",    "QUANTIZE",  "SCALE LOCK");
    lnf.drawModuleFrame (g, zSynth,    "04",    "SYNTH",     "8 ENGINES");
    lnf.drawModuleFrame (g, zFilter,   "01",    "FILTER",    "BP 1-12P");
    lnf.drawModuleFrame (g, zImpulse,  "02",    "IMPULSE",   "ONSET+PITCH");
    lnf.drawModuleFrame (g, zEvents,   "02-03", "EVENTS",    "T-6S");
    lnf.drawModuleFrame (g, zMix,      "06",    "MIX",       "OUT L/R");
    lnf.drawModuleFrame (g, zFx,       "05",    "EFFECTS",   "MASTER");

    // Voice rail label
    g.setFont (lnf.grot (8.5f, 0.14f));
    g.setColour (lnf.ink (0.55f));
    g.drawText ("VOICES", zVoices.getX(), zVoices.getY(), zVoices.getWidth(), 9,
                juce::Justification::centred, false);
}

void NewProjectAudioProcessorEditor::drawCaptions (juce::Graphics& g) const
{
    const int b = selectedBand;

    // ── Quantizer ──
    lnf.drawKnobCaption (g, knobScaleRoot.getBounds(), "SCALE",
                         fmtNote ((int) std::round (readParam (kScaleRootId))));
    lnf.drawKnobCaption (g, knobSnap.getBounds(), "SNAP",
                         fmtPercent01 ((readParam (bandSnapId (b)) + 1.f) * .5f));
    g.setFont (lnf.grot (8.5f, 0.10f));
    g.setColour (lnf.ink (0.52f));
    g.drawText ("NOTES", 788, 117, 168, 9, juce::Justification::centred, false);

    // ── Filter ──
    lnf.drawKnobCaption (g, knobFreq.getBounds(),  "FREQ",  fmtHz (readParam (bandFreqId (b))));
    lnf.drawKnobCaption (g, knobRes.getBounds(),   "RES",   fmtPercent01 (readParam (bandResId (b))));
    lnf.drawKnobCaption (g, knobSlope.getBounds(), "SLOPE", fmtInt (readParam (bandPoleId (b))) + "P");

    // ── Impulse detection ──
    lnf.drawKnobCaption (g, knobThreshold.getBounds(),   "THRESHOLD",   juce::String ((int) std::round (readParam (bandGateId (b)))) + "DB", 64);
    lnf.drawKnobCaption (g, knobSensitivity.getBounds(), "SENSITIVITY", fmtPercent01 (readParam (bandSensId (b))), 64);
    lnf.drawKnobCaption (g, knobTolerance.getBounds(),   "TOLERANCE",   fmtPercent01 (readParam (bandTolId (b))), 64);
    lnf.drawKnobCaption (g, knobConfidence.getBounds(),  "CONFIDENCE",  fmtPercent01 (readParam (bandConfId (b))), 64);

    // Gate activity — which voices the environment is triggering right now
    {
        g.setFont (lnf.grot (8.0f, 0.10f));
        g.setColour (lnf.ink (0.45f));
        g.drawText ("GATE", 26, 590, 28, 9, juce::Justification::centredLeft, false);
        for (int i = 0; i < 8; ++i)
        {
            juce::Rectangle<float> cell (60.0f + (float) i * 25.0f, 586.0f, 23.0f, 16.0f);
            const bool en = audioProcessor.isBandEnabled (i);
            const bool open = en && audioProcessor.getBandGateOpen (i);
            const float env = juce::jlimit (0.0f, 1.0f, audioProcessor.getBandEnvLevel (i));
            if (open)
            {
                g.setColour (kBandColours[i].withAlpha (0.25f + 0.75f * env));
                g.fillRect (cell);
            }
            g.setColour (open ? kBandColours[i] : lnf.ink (en ? 0.3f : 0.12f));
            g.drawRect (cell, 1.0f);
        }
    }

    // ── Synth ──
    g.setFont (lnf.grot (8.5f, 0.10f));
    g.setColour (lnf.ink (0.52f));
    g.drawText ("ENGINE", 604, 194, 172, 9, juce::Justification::centred, false);

    lnf.drawKnobCaption (g, knobWaveform.getBounds(), "MORPH",
                         fmtPercent01 ((readParam (bandWaveMixId (b)) + 1.f) * .5f));
    lnf.drawKnobCaption (g, knobTimbre.getBounds(), "TIMBRE",
                         fmtPercent01 (readParam (bandTimbreId (b))));

    // ADSR — letters below, live value above while dragging
    {
        struct AI { const juce::Slider* k; const char* l; bool isT; };
        const AI adsr[4] = { { &knobA, "A", true }, { &knobD, "D", true },
                             { &knobS, "S", false }, { &knobR, "R", true } };
        for (auto& a : adsr)
        {
            auto kb = a.k->getBounds();
            g.setFont (lnf.grot (8.5f, 0.10f));
            g.setColour (lnf.ink (0.52f));
            g.drawText (a.l, kb.getX() - 8, kb.getBottom() + 2, kb.getWidth() + 16, 9,
                        juce::Justification::centred, false);
            if (a.k->isMouseButtonDown())
            {
                const float val = (float) a.k->getValue();
                g.setFont (lnf.mono (8.0f));
                g.setColour (lnf.ink (0.95f));
                g.drawText (a.isT ? fmtSeconds (val) : fmtPercent01 (val),
                            kb.getX() - 14, kb.getY() - 12, kb.getWidth() + 28, 10,
                            juce::Justification::centred, false);
            }
        }
    }

    lnf.drawKnobCaption (g, knobHP.getBounds(),    "HP",    fmtHz (readParam (bandHPId (b))), 42);
    lnf.drawKnobCaption (g, knobTP.getBounds(),    "LP",    fmtHz (readParam (bandLPId (b))), 42);
    lnf.drawKnobCaption (g, knobTone.getBounds(),  "TONE",  fmtPercent01 (readParam (bandToneId (b))), 42);
    lnf.drawKnobCaption (g, knobNoise.getBounds(), "NOISE", fmtPercent01 (readParam (bandNoiseId (b))), 42);

    // ── Effects grid ──
    lnf.drawKnobCaption (g, knobWidth.getBounds(),     "WIDTH",    fmtPercent01 (readParam (kWidthId)), 64);
    lnf.drawKnobCaption (g, knobDryWet.getBounds(),    "DRY/WET",  fmtPercent01 (readParam (kDryWetId)), 64);
    lnf.drawKnobCaption (g, knobRandom.getBounds(),    "RANDOM",   fmtPercent01 (readParam (kRandomId)), 64);
    lnf.drawKnobCaption (g, knobDrive.getBounds(),     "DRIVE",    fmtPercent01 (readParam (kDriveId)), 64);
    lnf.drawKnobCaption (g, knobDelayMix.getBounds(),  "DELAY",    fmtPercent01 (readParam (kDelayMixId)), 64);
    lnf.drawKnobCaption (g, knobDelayTime.getBounds(), "TIME",     juce::String ((int) std::round (readParam (kDelayTimeId))) + "MS", 64);
    lnf.drawKnobCaption (g, knobDelayFb.getBounds(),   "FEEDBACK", fmtPercent01 (readParam (kDelayFbId) / 0.95f), 64);
    lnf.drawKnobCaption (g, knobReverb.getBounds(),    "REVERB",   fmtPercent01 (readParam (kReverbId)), 64);
    lnf.drawKnobCaption (g, knobComp.getBounds(),      "GLUE",     fmtPercent01 (readParam (kCompId)), 64);
}

void NewProjectAudioProcessorEditor::createToucanDrawable(){
    if(auto xml=juce::XmlDocument::parse(juce::String(toucanSvg))) toucanDrawable=juce::Drawable::createFromSVG(*xml); }
