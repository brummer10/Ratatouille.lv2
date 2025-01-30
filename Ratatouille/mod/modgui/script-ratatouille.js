
function (event, funcs)
{
    function check_neural_model(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=SlotA]').text ('-- choose a NAM/AIDA-X model --');
        else
        {
            var string = value.split('\\').pop().split('/').pop();
            icon.find ('[rata-role=SlotA]').text (string.substring(0, 48));
        }
    }
    function check_neural_model1(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=SlotB]').text ('-- choose a NAM/AIDA-X model --');
        else
        {
            var string = value.split('\\').pop().split('/').pop();
            icon.find ('[rata-role=SlotB]').text (string.substring(0, 48));
        }
    }
    function check_irfile(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=IrA]').text ('-- choose an IR file --');
        else
        {
            var string = value.split('\\').pop().split('/').pop();
            icon.find ('[rata-role=IrA]').text (string.substring(0, 48));
        }
    }
    function check_irfile1(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=IrB]').text ('-- choose an IR file --');
        else
        {
            var string = value.split('\\').pop().split('/').pop();
            icon.find ('[rata-role=IrB]').text (string.substring(0, 48));
        }
    }
    function set_latency(icon, value)
    {
        v = value.toFixed(2);
        icon.find ('[rata-role=latency]').text ('Latency: ' + `${v}` + 'ms');
    }

    if (event.type == 'start')
    {
    }
    else if (event.type == 'change')
    {
        if (event.uri == 'urn:brummer:ratatouille#Neural_Model')
            check_neural_model(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#Neural_Model1')
            check_neural_model1(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#irfile')
            check_irfile(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#irfile1')
            check_irfile1(event.icon, event.value);
        else if (event.symbol == 'ms_latency')
            set_latency(event.icon, event.value);

    }
}
