%data1 = voltage
%data2 = inputCurrent
%data3 = outputCurrent
%data4 = state
function testPlot(data, time, labels, testName, xMax)
    % Find the maximum value for output current (for y-axes):
    iOutMax = max(data(:,3)) * 1.1;
    iInMax  = max(data(:,2)) * 1.1;

    newFig = figure; % create a new figure to plot on
    set(newFig, 'Color', 'white')
                
     iCurAxis = axes('YAxisLocation',  'right',  ...
                     'Color',          'white',   ...
                     'XLim',           [0, xMax], ...
                     'XColor',         'black', ...
                     'XTickLabelMode', 'Manual', ... % remove xticks
                     'XTick',           [],      ...
                     'YColor',          [0 .5 0],       ...
                     'YLim',            [0, iInMax], ...
                     'YTick',           [0; iInMax/10; iInMax], ...
                     'YTickMode',       'auto');
    
    oCurAxis = axes('YAxisLocation',    'right',     ...
                    'Color',            'none',     ...
                    'XLim',             [0, xMax], ...
                    'XColor',           'black',     ...
                    'XTickLabelMode',   'Manual',    ... % remove xticks
                    'XTick',            [],         ...
                    'YColor',           'red',       ...
                    'YLim',             [0, iOutMax], ...
                    'YTick',            [0; iOutMax/10; iOutMax], ...
                    'YTickMode',        'auto');
                
    voltAxis = axes('YAxisLocation',  'left',      ...
                    'Color',          'none',      ...
                    'XLim',           [0, xMax], ...
                    'XColor',         'black',     ...
                    'YColor',         'b',         ...
                    'YLim',            [0, 3.6],   ...
                    'YTick',           [0; .5; 4], ...
                    'YTickMode',       'auto');
    
    set(iCurAxis,'Position',get(iCurAxis,'Position') + [-.04, 0, +.04, 0])
    set(oCurAxis,'Position',get(oCurAxis,'Position') + [-.04, 0, +.04, 0])
    set(voltAxis,'Position',get(voltAxis,'Position') + [-.04, 0, +.04, 0])
    linkaxes([oCurAxis, iCurAxis],'xy') % link the y axes of output currents
    linkaxes([iCurAxis, oCurAxis, voltAxis],'x') % link the x axes for proper zooming
    
    t1 = title(testName(1));
    set(t1,{'FontSize'},{10.0})
    
    timeLabel   = xlabel(voltAxis,  'Time (ms)');
    voltLabel   = ylabel(voltAxis,  labels(1));
    inCurLabel  = ylabel(iCurAxis,  labels(2));
    outCurLabel = ylabel(oCurAxis,  labels(3));
    
    set(inCurLabel,  'Units', 'Normalized');
    set(outCurLabel, 'Units', 'Normalized');
    set(voltLabel,   'Units', 'Normalized');
    
    set(outCurLabel, 'VerticalAlignment', 'top');
    set(inCurLabel,  'VerticalAlignment', 'top');
    set(voltLabel,   'VerticalAlignment', 'bottom');
    
    set(inCurLabel,  'Position', get(inCurLabel, 'Position') + [0 -.25 0]);
    set(outCurLabel, 'Position', get(outCurLabel,'Position') + [0 +.25 0]);
    
    inCurLine  = line(time, data(:,2),'Parent',iCurAxis,'Color',[0 .5 0]);
    outCurLine = line(time, data(:,3),'Parent',oCurAxis, 'Color','r');
    voltLine   = line(time, data(:,1),'Parent',voltAxis,'Color', 'b'); % plot voltage
    stateLine  = line(time, data(:,4),'Parent',voltAxis,'Color','black','LineWidth',3);
    
    return
    
    figure; % create a new figure to plot on
    voltLine = line(time, data(:,1)); % plot voltage
    voltAxis = gca;
    set(voltAxis, 'XColor', 'black');
    set(voltAxis, 'YColor', 'blue');
    
    ax1_pos = get(voltAxis,'Position');
    curAxis = axes('Position',ax1_pos,...
                   'YAxisLocation','right',...
                   'Color','none');
    set(curAxis, 'XTickLabelMode', 'Manual');  %remove the redundant ticks from x axis
    set(curAxis, 'XTick', []);
    inCurLine  = line(time, data(:,2),'Parent',curAxis, 'Color',[0 .5 0]);
    outCurLine = line(time, data(:,3),'Parent',curAxis, 'Color','r');
    stateLine  = line(time, data(:,4),'Parent',voltAxis,'Color','black','LineWidth',3);
    
    ylim(voltAxis, [0, 3.6]);
    ylim(curAxis,  [0, maxCurrent*1.1]);
    
    return
    
    newFigure = figure();
    hold on
    
    [gAxes1, g2, g3] = plotyy(time,data(:,2),time,data(:,1)); %(inCurrent / outCurrent)
    set(g2, {'Color'}, {[0 .5 0]}) % set inCurrent for dark green
    set(g3, {'Color'}, {'r'}) % set outCurrent red
    %set(gAxes1,{'ycolor'},{'b';[0 .5 0]})
    xlim(gAxes1(1), [0, xMax]);
    xlim(gAxes1(2), [0, xMax]);
    ylim(gAxes1(1), [0, maxCurrent*1.1]);
    ylim(gAxes1(2), [0, maxCurrent*1.1]);
    inCurLabel = ylabel(gAxes1(1), labels(2));
    outCurLabel = ylabel(gAxes1(2), labels(3));
    set(outCurLabel, 'Color', 'r')
   
    set(gAxes1(2),'Position',[0.1,0.11,0.775,0.815]);
    %offset = 50;
    %posFig = get(gcf,'Position'); %get figure position
    %widFig = get(gcf,'Width'); %get figure width
    posLabel = get(get(gAxes1(2),'YLabel'),'Position'); %get label
    %posFigNew = posFig + [-offset 0 offset 0]; %resize figure
    %widFigNew = widFig + offset;
    %set(gcf,'Position',posFigNew)
    %set(gcf,'Width',widFigNew)
    %set(get(gAxes1(2),'YLabel'),'Position',posLabel+[-offset 0 0]) % move label
    %set(get(gAxes1(2),'YLabel'),'Position',posLabel+[4 0 0])
    set(get(gAxes1(2),'YLabel'),'Units','Normalized','Position',[1.11 .5 1])
    
    [gAxes2, g1, g4] = plotyy(time,data(:,3),time,data(:,4)); %(volts / state)
    set(g1, {'Color'}, {'b'})
    set(g4, {'Color'}, {'black'})
    set(g4, {'LineWidth'}, {2})
    
    set(newFigure,{'Color'},{'w'})
    set(gAxes2,{'ycolor'},{'b';'r'})
    
%    set(gAxes2,'Position',[.13,.8,.775,.21])
    %g4 = plot(time,data(:,4), 'black', 'LineWidth', 2); %(state)
    xlim(gAxes2(1), [0, xMax]);
    xlim(gAxes2(2), [0, xMax]);
    ylim(gAxes2(1), [0, 3.6]);
    %ylim(gAxes2(4), [0, 3.6]);
    voltLabel = ylabel(gAxes2(1), labels(1));
    stateLabel = ylabel(gAxes2(2), labels(2));
    xlabel('Time (ms)');
    t1 = title(testName(1));
    set(t1,{'FontSize'},{10.0})
    
    set(gAxes2(1), 'ytick', [0; .5; 4]);
    set(gAxes2(2), 'ytick', [0; .5; 4]);
    
    set(gAxes2(1), 'YTickMode', 'auto');
    set(gAxes2(2), 'YTickMode', 'auto');
    %set(gcf, 'Position', get(0,'Screensize')); % Maximize figure.
    hold off;
    return;
end

