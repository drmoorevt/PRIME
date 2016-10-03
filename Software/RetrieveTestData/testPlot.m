%data1 = voltage
%data2 = inputCurrent
%data3 = outputCurrent
%data4 = state
function testPlot(data, time, labels, testName, xMax)
    % Find the maximum value for output current (for y-axes):
    maxCurrent = max(data(:,3));

    newFigure = figure();
    hold on
    
    [gAxes, g2, g3] = plotyy(time,data(:,2),time,data(:,3)); %(inCurrent / outCurrent)
    set(g2, {'Color'}, {[0 .5 0]}) % set outCurrent for dark green
    set(g3, {'Color'}, {'r'}) % set inCurrent red
    set(gAxes,{'ycolor'},{'b';[0 .5 0]})
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    ylim(gAxes(2), [0, maxCurrent*1.1]);
    voltLabel = ylabel(gAxes(1), labels(1));
    inCurLabel = ylabel(gAxes(2), labels(2));
   
    set(gAxes(1),'Position',[0.1,0.11,0.775,0.815]);
    %offset = 50;
    %posFig = get(gcf,'Position'); %get figure position
    %widFig = get(gcf,'Width'); %get figure width
    posLabel = get(get(gAxes(2),'YLabel'),'Position'); %get label
    %posFigNew = posFig + [-offset 0 offset 0]; %resize figure
    %widFigNew = widFig + offset;
    %set(gcf,'Position',posFigNew)
    %set(gcf,'Width',widFigNew)
    %set(get(gAxes(2),'YLabel'),'Position',posLabel+[-offset 0 0]) % move label
    %set(get(gAxes(2),'YLabel'),'Position',posLabel+[4 0 0])
    set(get(gAxes(2),'YLabel'),'Units','Normalized','Position',[1.11 .5 1])
    
    [gAxes, g1, g4] = plotyy(time,data(:,1),time,data(:,4)); %(outCurrent)
    set(g1, {'Color'}, {'b'})
    set(g4, {'Color'}, {'black'})
    set(g4, {'LineWidth'}, {2})
    
    set(newFigure,{'Color'},{'w'})
    set(gAxes,{'ycolor'},{'b';'r'})
    
%    set(gAxes,'Position',[.13,.8,.775,.21])
    %g4 = plot(time,data(:,4), 'black', 'LineWidth', 2); %(state)
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    outCurLabel = ylabel(gAxes(2), labels(3));
    xlabel('Time (ms)');
    t1 = title(testName(1));
    set(t1,{'FontSize'},{10.0})
    
    set(gAxes(1), 'ytick', [0; .5; 4]);
    set(gAxes(2), 'ytick', [0; .5; 4]);
    set(gAxes(1), 'YTickMode', 'auto');
    set(gAxes(2), 'YTickMode', 'auto');
    set(gcf, 'Position', get(0,'Screensize')); % Maximize figure.
    hold off;
    return;
end

