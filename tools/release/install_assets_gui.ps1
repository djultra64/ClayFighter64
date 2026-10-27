$BasePath = $env:CLAY_BASE_ROM
$SculptorPath = $env:CLAY_SC_ROM
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$form = New-Object System.Windows.Forms.Form
$form.Text = 'ClayFighter 64 - Install Assets'
$form.StartPosition = 'CenterScreen'
$form.ClientSize = New-Object System.Drawing.Size(620, 270)
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$intro = New-Object System.Windows.Forms.Label
$intro.Text = 'Both supported USA ROMs are required. Select any missing files below.'
$intro.SetBounds(20, 15, 580, 35)
$form.Controls.Add($intro)
$fields = @()
$names = @('ClayFighter 63 1/3 (USA)', "ClayFighter: Sculptor's Cut (USA)")
$values = @($BasePath, $SculptorPath)
for ($i = 0; $i -lt 2; $i++) {
    $label = New-Object System.Windows.Forms.Label
    $label.Text = $names[$i]
    $label.SetBounds(20, (55 + $i * 75), 570, 20)
    $form.Controls.Add($label)
    $field = New-Object System.Windows.Forms.TextBox
    $field.SetBounds(20, (78 + $i * 75), 465, 25)
    $field.Text = $values[$i]
    $form.Controls.Add($field)
    $fields += $field
    $browse = New-Object System.Windows.Forms.Button
    $browse.Text = 'Browse...'
    $browse.SetBounds(495, (76 + $i * 75), 105, 28)
    $browse.Tag = $field
    $browse.Add_Click({
        $dialog = New-Object System.Windows.Forms.OpenFileDialog
        $dialog.Title = 'Select your USA ROM'
        $dialog.Filter = 'N64 ROMs and ZIP archives|*.z64;*.v64;*.n64;*.c64;*.c;*.zip|All files|*.*'
        if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { $this.Tag.Text = $dialog.FileName }
        $dialog.Dispose()
    })
    $form.Controls.Add($browse)
}
$install = New-Object System.Windows.Forms.Button
$install.Text = 'Install'
$install.SetBounds(390, 225, 100, 30)
$install.Add_Click({
    if ([string]::IsNullOrWhiteSpace($fields[0].Text) -or [string]::IsNullOrWhiteSpace($fields[1].Text)) {
        [System.Windows.Forms.MessageBox]::Show('Select both ROMs to continue.', 'ClayFighter 64') | Out-Null
        return
    }
    $form.DialogResult = [System.Windows.Forms.DialogResult]::OK
    $form.Close()
})
$form.Controls.Add($install)
$cancel = New-Object System.Windows.Forms.Button
$cancel.Text = 'Cancel'
$cancel.SetBounds(500, 225, 100, 30)
$cancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
$form.Controls.Add($cancel)
$form.AcceptButton = $install
$form.CancelButton = $cancel
if ($form.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
    ConvertTo-Json -InputObject @($fields[0].Text, $fields[1].Text) -Compress
}
$form.Dispose()
