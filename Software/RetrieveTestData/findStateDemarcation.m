function [indexOfChange] = findStateDemarcation(data, startIndex)
  numElem  = numel(data(:,4));
  startVal = data(startIndex, 4);
  while (startIndex < numElem)
      if (startVal ~= data(startIndex, 4))
          break;
      else
        startIndex = startIndex + 1;
      end
  end
  indexOfChange = startIndex;
end

