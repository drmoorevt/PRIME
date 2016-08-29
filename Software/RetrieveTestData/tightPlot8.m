%usage:
%tightPlot3(name,chans,time,avgData(:,:,1,1),avgData(:,:,1,2),avgData(:,:,1,3),50)
function tightPlot8(name,chans,time,data1,data2,xMax, y2Max)
    close all
    hFig = figure(1);
    set(hFig, 'Position', [1000 50 637.5 825])    

    for i = 1:8
    subtightplot(2,4,i,[.01 .01], [.1 .1], [.1 .1]);
    %plot(time,data(:,[1,3,4],1,1));
    [gAxes, g1, g2] = plotyy(time,data1(:,1),time,data1(:,3)); %(outCurrent)
    hold on;
    set(figure(1),{'Color'},{'w'})
    set(g2, {'Color'}, {'r'})
    set(gAxes,{'ycolor'},{'b';'r'})
%    set(gAxes,'Position',[.13,.8,.775,.21])
    plot(time,data1(:,4), 'black', 'LineWidth', 2); %(state)
    xlim(gAxes(1), [0, xMax]);
    xlim(gAxes(2), [0, xMax]);
    ylim(gAxes(1), [0, 3.6]);
    ylim(gAxes(2), [0, y2Max]);
    ylabel(gAxes(1), chans(1));
    ylabel(gAxes(2), 'Domain Current (mA)');
    t1 = title(name(1));
    set(t1,{'FontSize'},{10.0})
    set(gAxes(1), 'ytick', [0; .5; 4]);
    set(gAxes(2), 'ytick', [0; .5; 4]);
    set(gAxes(1), 'YTickMode', 'auto');
    set(gAxes(2), 'YTickMode', 'auto');
    end
    
    xlabel('Time (ms)');
    legend(chans([1,4,3]),'Location','SouthOutside','Orientation','horizontal')
    
    hold off
end